/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <QApplication>
#include <QList>
#include <QQuickView>
#include <QQuickImageProvider>
#include <QQmlContext>
#include <QScreen>
#include <QDir>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "wayland-screenshooter-client-protocol.h"

static const QEvent::Type ScreenshotEventType = (QEvent::Type)QEvent::registerEventType();

class Screenshooter;

class Screenshot
{
public:
    static Screenshot *create(Screenshooter *p, wl_shm *shm, QScreen *screen)
    {
        int width = screen->size().width();
        int height = screen->size().height();
        int stride = width * 4;
        int size = stride * height;

        char filename[] = "/tmp/orbital-screenshooter-shm-XXXXXX";
        int fd = mkstemp(filename);
        if (fd < 0) {
            qWarning("creating a buffer file for %d B failed: %m\n", size);
            return nullptr;
        }
        int flags = fcntl(fd, F_GETFD);
        if (flags != -1)
            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

        if (ftruncate(fd, size) < 0) {
            qWarning("ftruncate failed: %s", strerror(errno));
            close(fd);
            return nullptr;
        }

        uchar *data = (uchar *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        unlink(filename);
        if (data == (uchar *)MAP_FAILED) {
            qWarning("mmap failed: %m\n");
            close(fd);
            return nullptr;
        }

        Screenshot *shot = new Screenshot;

        shot->parent = p;
        wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        shot->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        shot->data = data;
        shot->screen = screen;
        close(fd);
        return shot;
    }

    Screenshooter *parent;
    QScreen *screen;
    wl_buffer *buffer;
    uchar *data;
    orbital_screenshot *screenshot;

    static const orbital_screenshot_listener s_listener;
};

class ScreenshotEvent : public QEvent
{
public:
    ScreenshotEvent(Screenshot *s)
        : QEvent(ScreenshotEventType)
        , shot(s)
    {
    }

    Screenshot *shot;
};

class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        *size = m_image.size();
        return m_image;
    }

    QImage m_image;
};

class Screenshooter : public QObject
{
    Q_OBJECT
public:
    Screenshooter()
        : QObject()
        , m_shooter(nullptr)
        , m_shm(nullptr)
    {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
        m_registry = wl_display_get_registry(m_display);
        wl_registry_add_listener(m_registry, &s_registryListener, this);
        wl_display_dispatch(m_display);
        wl_display_roundtrip(m_display);
        if (!m_shooter || !m_shm) {
            exit(1);
        }

        for (QScreen *screen: QGuiApplication::screens()) {
            Screenshot *screenshot = Screenshot::create(this, m_shm, screen);
            if (!screenshot) {
                exit(1);
            }
            m_screenshots << screenshot;
        }
        m_imageProvider = new ImageProvider;

        QQuickWindow::setDefaultAlphaBuffer(true);
        m_window = new QQuickView;
        m_window->setTitle("Orbital screenshooter");
        m_window->setResizeMode(QQuickView::SizeRootObjectToView);
        m_window->engine()->addImageProvider(QLatin1String("screenshoter"), m_imageProvider);
        m_window->rootContext()->setContextProperty("Screenshooter", this);
        m_window->setSource(QUrl("qrc:///screenshooter.qml"));
        m_window->create();
        connect(m_window->engine(), &QQmlEngine::quit, qApp, &QCoreApplication::quit);

        takeShot();
    }
    ~Screenshooter()
    {
    }
    bool event(QEvent *e) override
    {
        if (e->type() == ScreenshotEventType) {
            ScreenshotEvent *se = static_cast<ScreenshotEvent *>(e);
            m_pendingScreenshots.remove(se->shot);
            orbital_screenshot_destroy(se->shot->screenshot);
            tryDone();
            return true;
        }
        return QObject::event(e);
    }

    void tryDone()
    {
        if (!m_pendingScreenshots.isEmpty()) {
            return;
        }

        int width = 0;
        int height = 0;
        int minX, minY;
        minX = minY = INT_MAX;
        for (Screenshot *s: m_screenshots) {
            QRect geom = s->screen->geometry();
            minX = qMin(minX, geom.x());
            minY = qMin(minY, geom.y());
            width += geom.width();
            if (geom.height() > height) {
                height = geom.height();
            }
        }
        int stride = width * 4;
        uchar *data = new uchar[stride * height];
        memset(data, 0, stride * height);

        for (Screenshot *ss: m_screenshots) {
            QRect geom = ss->screen->geometry();
            int output_stride = geom.width() * 4;
            uchar *s = ss->data;
            uchar *d = data + (geom.y() - minY) * stride + (geom.x() - minX) * 4;

            for (int i = 0; i < geom.height(); i++) {
                memcpy(d, s, output_stride);
                d += stride;
                s += output_stride;
            }
            // fill the remaining lines with solid black
            for (int i = geom.height(); i < height; ++i) {
                for (uchar *line = d + 3; line < d + output_stride; line += 4) {
                    *line = 0xff;
                }
                d += stride;
            }
        }

        m_imageProvider->m_image = QImage(data, width, height, stride, QImage::Format_ARGB32);
        m_window->show();
        emit newShot();
    }

public slots:
    void takeShot()
    {
        m_window->hide();
        for (Screenshot *ss: m_screenshots) {
            m_pendingScreenshots << ss;
            wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", ss->screen));
            ss->screenshot = orbital_screenshooter_shoot(m_shooter, output, ss->buffer);
            orbital_screenshot_add_listener(ss->screenshot, &Screenshot::s_listener, ss);
        }
    }
    void save(const QString &path)
    {
        QString p = path;
        p.remove(0, 7); // Remove the "file://"
        if (!m_imageProvider->m_image.save(p)) {
            m_imageProvider->m_image.save(p + ".jpg");
        }
    }

signals:
    void newShot();

private:
    wl_display *m_display;
    wl_registry *m_registry;
    orbital_screenshooter *m_shooter;
    wl_shm *m_shm;
    QQuickView *m_window;
    QList<Screenshot *> m_screenshots;
    QSet<Screenshot *> m_pendingScreenshots;
    ImageProvider *m_imageProvider;

    static const wl_registry_listener s_registryListener;
};

const wl_registry_listener Screenshooter::s_registryListener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        Screenshooter *shooter = static_cast<Screenshooter *>(data);

        if (strcmp(interface, "orbital_screenshooter") == 0) {
            shooter->m_shooter = static_cast<orbital_screenshooter *>(wl_registry_bind(registry, id, &orbital_screenshooter_interface, version));
        } else if (strcmp(interface, "wl_shm") == 0) {
            shooter->m_shm = static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, version));
        }
    },
    [](void *, wl_registry *registry, uint32_t id) {}
};

const orbital_screenshot_listener Screenshot::s_listener = {
    [](void *data, orbital_screenshot *s) {
        Screenshot *ss = static_cast<Screenshot *>(data);
        qApp->postEvent(ss->parent, new ScreenshotEvent(ss));
    }
};

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("QT_MESSAGE_PATTERN", "[orbital-screenshooter %{type}] %{message}", 1);

    QApplication app(argc, argv);
    Screenshooter shooter;

    return app.exec();
}

#include "main.moc"

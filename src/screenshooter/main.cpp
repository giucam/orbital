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

class Screenshot
{
public:
    static Screenshot *create(wl_shm *shm, QScreen *screen)
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

        wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        shot->m_buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        shot->m_data = data;
        shot->m_image = QImage(data, width, height, stride, QImage::Format_ARGB32);

        close(fd);
        return shot;
    }

    wl_buffer *buffer() const { return m_buffer; }
    const QImage &image() const { return m_image; }

private:
    Screenshot() { }

    wl_buffer *m_buffer;
    uchar *m_data;
    QImage m_image;
};

class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider(Screenshot *shot)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_shot(shot)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        const QImage &image = m_shot->image();
        *size = image.size();
        return image;
    }

private:
    Screenshot *m_shot;
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

        screenshooter_add_listener(m_shooter, &s_listener, this);

        QScreen *screen = QGuiApplication::screens().first();
        m_screenshot = Screenshot::create(m_shm, screen);
        if (!m_screenshot) {
            exit(1);
        }

        QQuickWindow::setDefaultAlphaBuffer(true);
        m_window = new QQuickView;
        m_window->setTitle("Orbital screenshooter");
        m_window->setResizeMode(QQuickView::SizeRootObjectToView);
        m_window->engine()->addImageProvider(QLatin1String("screenshoter"), new ImageProvider(m_screenshot));
        m_window->rootContext()->setContextProperty("Screenshooter", this);
        m_window->setSource(QUrl("qrc:///screenshooter.qml"));
        m_window->create();

        takeShot();
    }
    ~Screenshooter()
    {
    }

public slots:
    void done()
    {
        m_window->show();

        emit newShot();
    }

    void takeShot()
    {
        m_window->hide();
        wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", m_window->screen()));
        screenshooter_shoot(m_shooter, output, m_screenshot->buffer());
    }
    void save(const QString &path)
    {
        QString p = path;
        p.remove(0, 7); // Remove the "file://"
        if (!m_screenshot->image().save(p)) {
            m_screenshot->image().save(p + ".jpg");
        }
    }

signals:
    void newShot();

private:
    wl_display *m_display;
    wl_registry *m_registry;
    screenshooter *m_shooter;
    wl_shm *m_shm;
    QQuickView *m_window;
    Screenshot *m_screenshot;

    static const wl_registry_listener s_registryListener;
    static const screenshooter_listener s_listener;
};

const wl_registry_listener Screenshooter::s_registryListener = {
    [](void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        Screenshooter *shooter = static_cast<Screenshooter *>(data);

        if (strcmp(interface, "screenshooter") == 0) {
            shooter->m_shooter = static_cast<screenshooter *>(wl_registry_bind(registry, id, &screenshooter_interface, version));
        } else if (strcmp(interface, "wl_shm") == 0) {
            shooter->m_shm = static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, version));
        }
    },
    [](void *, wl_registry *registry, uint32_t id) {}
};

const screenshooter_listener Screenshooter::s_listener = {
    [](void *data, screenshooter *s) { QMetaObject::invokeMethod(static_cast<Screenshooter *>(data), "done"); }
};

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("QT_MESSAGE_PATTERN", "[orbital-splash %{type}] %{message}", 0);

    QApplication app(argc, argv);
    Screenshooter shooter;

    return app.exec();
}

#include "main.moc"

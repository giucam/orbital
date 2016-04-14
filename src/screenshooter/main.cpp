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
#include <QTemporaryFile>
#include <QProcess>
#include <QClipboard>
#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

#include "../client/utils.h"
#include "wayland-screenshooter-client-protocol.h"
#include "wayland-authorizer-client-protocol.h"

static const QEvent::Type ScreenshotEventType = (QEvent::Type)QEvent::registerEventType();

class Screenshooter;

class Screenshot
{
public:
    static Screenshot *create(Screenshooter *p, QScreen *screen, wl_shm *shm, int width, int height, int stride)
    {
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
        shot->width = width;
        shot->height = height;
        shot->stride = stride;
        shot->screen = screen;
        close(fd);
        return shot;
    }

    Screenshooter *parent;
    QScreen *screen;
    wl_buffer *buffer;
    int width, height, stride;
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


class SurfaceScreenshot : public QObject
{
    Q_OBJECT
public:
    SurfaceScreenshot(orbital_screenshooter *shooter, wl_shm *shm)
        : m_shm(shm)
    {
        m_screenshot = orbital_screenshooter_shoot_surface(shooter);
        static const orbital_surface_screenshot_listener listener = {
            wrapInterface(&SurfaceScreenshot::setup),
            wrapInterface(&SurfaceScreenshot::done),
            wrapInterface(&SurfaceScreenshot::failed),
        };
        orbital_surface_screenshot_add_listener(m_screenshot, &listener, this);
    }

    void setup(orbital_surface_screenshot *, int32_t width, int32_t height, int32_t stride, int32_t format)
    {
        m_ss = Screenshot::create(nullptr, nullptr, m_shm, width, height, stride);
        orbital_surface_screenshot_shoot(m_screenshot, m_ss->buffer);
    }

    void done(orbital_surface_screenshot *)
    {
        emit taken(QImage(m_ss->data, m_ss->width, m_ss->height, m_ss->stride, QImage::Format_ARGB32));
    }

    void failed(orbital_surface_screenshot *)
    {
        emit taken(QImage());
    }

signals:
    void taken(const QImage &image);

private:
    wl_shm *m_shm;
    orbital_surface_screenshot *m_screenshot;
    Screenshot *m_ss;
};

class Screenshooter : public QObject
{
    Q_OBJECT
public:
    Screenshooter()
        : QObject()
        , m_shooter(nullptr)
        , m_shm(nullptr)
        , m_authorized(false)
    {
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
        m_registry = wl_display_get_registry(m_display);

        static const wl_registry_listener listener = {
            wrapInterface(&Screenshooter::global),
            wrapInterface(&Screenshooter::globalRemove)
        };

        wl_registry_add_listener(m_registry, &listener, this);
        wl_display_dispatch(m_display);
        wl_display_roundtrip(m_display);
        if (!m_shooter || !m_shm) {
            exit(1);
        }

        foreach (QScreen *screen, QGuiApplication::screens()) {
            int width = screen->size().width() * screen->devicePixelRatio();
            int height = screen->size().height() * screen->devicePixelRatio();
            int stride = width * 4;
            Screenshot *screenshot = Screenshot::create(this, screen, m_shm, width, height, stride);
            if (!screenshot) {
                exit(1);
            }
            m_screenshots << screenshot;
        }
        m_imageProvider = new ImageProvider;

        QQuickWindow::setDefaultAlphaBuffer(true);
        m_window = new QQuickView;
        m_window->setTitle(QStringLiteral("Orbital screenshooter"));
        m_window->setResizeMode(QQuickView::SizeRootObjectToView);
        m_window->engine()->addImageProvider(QStringLiteral("screenshoter"), m_imageProvider);
        m_window->rootContext()->setContextProperty(QStringLiteral("Screenshooter"), this);
        m_window->setSource(QUrl(QStringLiteral("qrc:///screenshooter.qml")));
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
        foreach (Screenshot *s, m_screenshots) {
            qreal ratio = s->screen->devicePixelRatio();
            QRect geom = s->screen->geometry();
            minX = qMin(minX, int(geom.x() * ratio));
            minY = qMin(minY, int(geom.y() * ratio));
            width += geom.width() * ratio;
            if (geom.height() * ratio > height) {
                height = geom.height() * ratio;
            }
        }
        int stride = width * 4;
        uchar *data = new uchar[stride * height];
        memset(data, 0, stride * height);

        foreach (Screenshot *ss, m_screenshots) {
            qreal ratio = ss->screen->devicePixelRatio();
            QRect geom = ss->screen->geometry();
            int output_stride = geom.width() * ratio * 4;
            uchar *s = ss->data;
            uchar *d = data + (int(geom.y() * ratio) - minY) * stride + (int(geom.x() * ratio) - minX) * 4;

            for (int i = 0; i < geom.height() * ratio; i++) {
                memcpy(d, s, output_stride);
                d += stride;
                s += output_stride;
            }
            // fill the remaining lines with solid black
            for (int i = geom.height() * ratio; i < height; ++i) {
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
        foreach (Screenshot *ss, m_screenshots) {
            m_pendingScreenshots << ss;
            wl_output *output = static_cast<wl_output *>(QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", ss->screen));
            ss->screenshot = orbital_screenshooter_shoot(m_shooter, output, ss->buffer);
            orbital_screenshot_add_listener(ss->screenshot, &Screenshot::s_listener, ss);
        }
    }
    void takeSurfaceShot()
    {
        m_window->hide();
        auto *ss = new SurfaceScreenshot(m_shooter, m_shm);
        connect(ss, &SurfaceScreenshot::taken, this, [this, ss](const QImage &img)
        {
            QImage image(img.size(), QImage::Format_ARGB32);
            for (int i = 0; i < img.height(); ++i) {
                uchar *dst = image.scanLine(i);
                const uchar *src = img.scanLine(i);
                for (int j = 0; j < img.bytesPerLine(); j += 4) {
                    dst[j + 0] = src[j + 2];
                    dst[j + 1] = src[j + 1];
                    dst[j + 2] = src[j + 0];
                    dst[j + 3] = src[j + 3];
                }
            }
            m_imageProvider->m_image = image;
            m_window->show();
            emit newShot();
            delete ss;
        });
    }
    void save(const QString &path)
    {
        QString p = path;
        p.remove(0, 7); // Remove the "file://"
        if (!m_imageProvider->m_image.save(p)) {
            m_imageProvider->m_image.save(p + QLatin1String(".jpg"));
        }
    }
    void upload()
    {
        QTemporaryFile *file = new QTemporaryFile(QStringLiteral("/tmp/orbital-screenshooter-XXXXXX.jpg"));
        if (!m_imageProvider->m_image.save(file)) {
            qWarning("Cannot save the screenshot to a temporary file");
            delete file;
            return;
        }
        emit uploadOutput(QStringLiteral("Uploading..."));

        QProcess *proc = new QProcess;
        QProcessEnvironment env;
        proc->setProcessEnvironment(env);
        proc->start(QStringLiteral("sh " LIBEXEC_PATH "/imgur %1").arg(file->fileName()));
        connect(proc, (void (QProcess::*)(int))&QProcess::finished, [this, proc, file]() {
            QString stdout(QString::fromUtf8(proc->readAllStandardOutput()));
            QString stderr(QString::fromUtf8(proc->readAllStandardError()));

            QClipboard *cb = QGuiApplication::clipboard();
            cb->setText(stdout);

            QString s = QStringLiteral("Image uploaded: %1\n%2").arg(stdout, stderr);
            emit uploadOutput(s);
            delete file;
        });
    }

signals:
    void newShot();
    void uploadOutput(const QString &output);

private:
    void global(wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
    {
#define registry_bind(type, v) static_cast<type *>(wl_registry_bind(registry, id, &type ## _interface, qMin(version, v)))

        if (strcmp(interface, "orbital_screenshooter") == 0) {
            m_shooter = registry_bind(orbital_screenshooter, 1u);
        } else if (strcmp(interface, "wl_shm") == 0) {
            m_shm = registry_bind(wl_shm, 1u);
        } else if (strcmp(interface, "orbital_authorizer") == 0) {
            wl_event_queue *queue = wl_display_create_queue(m_display);

            orbital_authorizer *auth = registry_bind(orbital_authorizer, 1u);
            wl_proxy_set_queue((wl_proxy *)auth, queue);
            orbital_authorizer_feedback *feedback = orbital_authorizer_authorize(auth, "orbital_screenshooter");

            static const orbital_authorizer_feedback_listener listener = {
                wrapInterface(&Screenshooter::authGranted),
                wrapInterface(&Screenshooter::authDenied)
            };
            orbital_authorizer_feedback_add_listener(feedback, &listener, this);

            int ret = 0;
            while (!m_authorized && ret >= 0) {
                ret = wl_display_dispatch_queue(m_display, queue);
            }

            orbital_authorizer_feedback_destroy(feedback);
            orbital_authorizer_destroy(auth);
            wl_event_queue_destroy(queue);
        }
    }
    void globalRemove(wl_registry *registry, uint32_t id) {}

    void authGranted(orbital_authorizer_feedback *feedback)
    {
        m_authorized = true;
    }

    void authDenied(orbital_authorizer_feedback *feedback)
    {
        qWarning("Fatal! Authorization to bind the orbital_screenshooter global interface denied.");
        exit(1);
    }

    wl_display *m_display;
    wl_registry *m_registry;
    orbital_screenshooter *m_shooter;
    wl_shm *m_shm;
    QQuickView *m_window;
    QList<Screenshot *> m_screenshots;
    QSet<Screenshot *> m_pendingScreenshots;
    ImageProvider *m_imageProvider;
    bool m_authorized;
};

const orbital_screenshot_listener Screenshot::s_listener = {
    [](void *data, orbital_screenshot *s) {
        Screenshot *ss = static_cast<Screenshot *>(data);
        qApp->postEvent(ss->parent, new ScreenshotEvent(ss));
    },
    [](void *data, orbital_screenshot *s) {
        Screenshot *ss = static_cast<Screenshot *>(data);
        qApp->postEvent(ss->parent, new ScreenshotEvent(ss));
    }
};

int main(int argc, char *argv[])
{
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT", "1", 1);

    QApplication app(argc, argv);
    Screenshooter shooter;

    return app.exec();
}

#include "main.moc"

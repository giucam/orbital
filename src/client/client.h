
#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QQmlListProperty>

class QQmlEngine;
class QQmlComponent;
class QWindow;

struct wl_display;
struct wl_registry;
struct wl_surface;
struct wl_registry_listener;

struct desktop_shell;
struct desktop_shell_listener;
struct desktop_shell_binding;
struct desktop_shell_window;

class ProcessLauncher;
class Window;

class Binding : public QObject
{
    Q_OBJECT
public:
    ~Binding();

signals:
    void triggered();

private:
    desktop_shell_binding *bind;
    friend class Client;
};

class Client : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Window> windows READ windows NOTIFY windowsChanged)
public:
    Client();
    ~Client();

    Q_INVOKABLE Binding *addKeyBinding(uint32_t key, uint32_t modifiers);

    QQmlListProperty<Window> windows();

signals:
    void windowsChanged();

private slots:
    void create();
    void createWindow();
    void windowRemoved(Window *w);

private:
    static void handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void configure(void *data, desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height);
    static void handlePrepareLockSurface(void *data, desktop_shell *desktop_shell);
    static void handleGrabCursor(void *data, desktop_shell *desktop_shell, uint32_t cursor);
    static void handleWindowAdded(void *data, desktop_shell *desktop_shell, desktop_shell_window *window, const char *title, int32_t state);

    static const wl_registry_listener s_registryListener;
    static const desktop_shell_listener s_shellListener;

    static int windowsCount(QQmlListProperty<Window> *prop);
    static Window *windowsAt(QQmlListProperty<Window> *prop, int index);

    wl_display *m_display;
    wl_registry *m_registry;
    int m_fd;
    desktop_shell *m_shell;
    ProcessLauncher *m_launcher;
    QWindow *m_grabWindow;
    QList<Binding *> m_bindings;

    QQmlEngine *m_engine;
    QQmlComponent *m_component;
    QObject *m_rootObject;

    Window *m_nextWindow;
    QList<Window *> m_windows;
};

#endif

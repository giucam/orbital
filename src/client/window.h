
#ifndef WINDOW_H
#define WINDOW_H

#include <QObject>

struct desktop_shell_window;
struct desktop_shell_window_listener;

class Window : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
public:
    Window(QObject *p = nullptr);
    ~Window();
    void init(desktop_shell_window *window);

    inline QString title() const { return m_title; }
    void setTitle(const QString &title);

    inline bool isActive() const { return m_active; }

    void setState(int32_t state);

public slots:
    void activate();
    void minimize();

signals:
    void destroyed(Window *w);
    void titleChanged();
    void activeChanged();

private:
    desktop_shell_window *m_window;
    QString m_title;
    bool m_active;

    static void set_active(void *data, desktop_shell_window *window, int32_t activated);
    static void removed(void *data, desktop_shell_window *window);
    static const desktop_shell_window_listener m_window_listener;
};

#endif

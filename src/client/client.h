/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QQmlListProperty>
#include <QSurfaceFormat>
#include <QElapsedTimer>

class QQmlEngine;
class QQmlComponent;
class QWindow;
class QQuickWindow;

struct wl_display;
struct wl_registry;
struct wl_surface;
struct wl_registry_listener;

struct desktop_shell;
struct desktop_shell_listener;
struct desktop_shell_binding;
struct desktop_shell_window;
struct desktop_shell_workspace;

class ProcessLauncher;
class Window;
class ShellUI;
class Grab;
class Workspace;

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
    Q_PROPERTY(QQmlListProperty<Workspace> workspaces READ workspaces NOTIFY workspacesChanged)
public:
    Client();
    ~Client();

    Q_INVOKABLE Binding *addKeyBinding(uint32_t key, uint32_t modifiers);

    QQmlListProperty<Window> windows();
    QQmlListProperty<Workspace> workspaces();

    void requestFocus(QWindow *window);

    Q_INVOKABLE void logOut();
    Q_INVOKABLE void poweroff();
    Q_INVOKABLE void reboot();

    static Grab *createGrab();
    static QQuickWindow *createUiWindow();

    static Client *client() { return s_client; }
    static QQmlEngine *qmlEngine() { return s_client->m_engine; }

    QQuickWindow *findWindow(wl_surface *surface) const;

public slots:
    void minimizeWindows();
    void restoreWindows();
    void addWorkspace();
    void selectWorkspace(Workspace *ws);

signals:
    void windowsChanged();
    void workspacesChanged();

private slots:
    void create();
    void windowRemoved(Window *w);
    void setGrabCursor();
    void ready();

private:
    static void handleGlobal(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void handleLoad(void *data, desktop_shell *shell);
    static void configure(void *data, desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height);
    static void handlePrepareLockSurface(void *data, desktop_shell *desktop_shell);
    static void handleGrabCursor(void *data, desktop_shell *desktop_shell, uint32_t cursor);
    static void handleWindowAdded(void *data, desktop_shell *desktop_shell, desktop_shell_window *window, const char *title, int32_t state);
    static void handleWorkspaceAdded(void *data, desktop_shell *desktop_shell, desktop_shell_workspace *ws, int active);

    static const wl_registry_listener s_registryListener;
    static const desktop_shell_listener s_shellListener;

    static int windowsCount(QQmlListProperty<Window> *prop);
    static Window *windowsAt(QQmlListProperty<Window> *prop, int index);
    static int workspacesCount(QQmlListProperty<Workspace> *prop);
    static Workspace *workspacesAt(QQmlListProperty<Workspace> *prop, int index);

    wl_display *m_display;
    wl_registry *m_registry;
    int m_fd;
    desktop_shell *m_shell;
    ProcessLauncher *m_launcher;
    QSurfaceFormat m_surfaceFormat;
    QWindow *m_grabWindow;
    QList<Binding *> m_bindings;
    QList<QQuickWindow *> m_uiWindows;
    QElapsedTimer m_elapsedTimer;

    QQmlEngine *m_engine;
    QQmlComponent *m_component;
    ShellUI *m_ui;

    QList<Window *> m_windows;
    QList<Workspace *> m_workspaces;

    uint32_t m_pendingGrabCursor;

    static Client *s_client;
};

#endif

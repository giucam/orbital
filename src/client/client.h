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
#include <QElapsedTimer>

class QQmlEngine;
class QQmlComponent;
class QWindow;
class QQuickWindow;
class QDBusInterface;

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
class ElementInfo;
class Service;
class StyleInfo;

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
    Q_PROPERTY(QQmlListProperty<ElementInfo> elementsInfo READ elementsInfo NOTIFY elementsInfoChanged)
    Q_PROPERTY(QQmlListProperty<StyleInfo> stylesInfo READ stylesInfo NOTIFY stylesInfoChanged)
public:
    Client();
    ~Client();

    Q_INVOKABLE Binding *addKeyBinding(uint32_t key, uint32_t modifiers);

    QQmlListProperty<Window> windows();
    QQmlListProperty<Workspace> workspaces();
    QQmlListProperty<ElementInfo> elementsInfo();
    QQmlListProperty<StyleInfo> stylesInfo();

    Q_INVOKABLE Service *service(const QString &name);

    Q_INVOKABLE void logOut();
    Q_INVOKABLE void poweroff();
    Q_INVOKABLE void reboot();

    static Grab *createGrab();
    static QQuickWindow *createUiWindow();

    static Client *client() { return s_client; }

    QQuickWindow *findWindow(wl_surface *surface) const;

public slots:
    void minimizeWindows();
    void restoreWindows();
    void addWorkspace(int n);
    void removeWorkspace(int n);
    void selectWorkspace(Workspace *ws);

signals:
    void windowsChanged();
    void workspacesChanged();
    void elementsInfoChanged();
    void stylesInfoChanged();

private slots:
    void create();
    void windowRemoved(Window *w);
    void setGrabCursor();
    void ready();

private:
    void handleGlobal(wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    void handleLoad(desktop_shell *shell);
    void handleConfigure(desktop_shell *shell, uint32_t edges, wl_surface *surf, int32_t width, int32_t height);
    void handlePrepareLockSurface(desktop_shell *desktop_shell);
    void handleGrabCursor(desktop_shell *desktop_shell, uint32_t cursor);
    void handleWindowAdded(desktop_shell *desktop_shell, desktop_shell_window *window, const char *title, int32_t state);
    void handleWorkspaceAdded(desktop_shell *desktop_shell, desktop_shell_workspace *ws, int active);

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
    QWindow *m_grabWindow;
    QList<Binding *> m_bindings;
    QList<QQuickWindow *> m_uiWindows;
    QElapsedTimer m_elapsedTimer;
    QDBusInterface *m_loginServiceInterface;
    QHash<QString, Service *> m_services;
    ShellUI *m_ui;

    QList<Window *> m_windows;
    QList<Workspace *> m_workspaces;

    uint32_t m_pendingGrabCursor;

    static Client *s_client;
};

#endif

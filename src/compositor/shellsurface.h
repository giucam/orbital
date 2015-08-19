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

#ifndef ORBITAL_SHELLSURFACE_H
#define ORBITAL_SHELLSURFACE_H

#include <functional>

#include <QHash>
#include <QObject>
#include <QRect>

#include "interface.h"
#include "utils.h"
#include "surface.h"

struct wl_client;
struct weston_surface;
struct weston_shell_client;

namespace Orbital
{

class Shell;
class ShellView;
class AbstractWorkspace;
class Output;
class Seat;
class Compositor;
class Workspace;
struct Listener;

class ShellSurface : public Object, public Surface::RoleHandler
{
    Q_OBJECT
public:
    ShellSurface(Shell *shell, Surface *surface);
    ~ShellSurface();

    typedef std::function<void (int, int)> ConfigureSender;
    enum class Type {
        None = 0,
        Toplevel = 1,
        Popup = 2,
        Transient = 3,
        XWayland = 4
    };
    enum class Edges {
        None = 0,
        Top = 1,
        Bottom = 2,
        Left = 4,
        Right = 8,
        TopLeft = Top | Left,
        BottomLeft = Bottom | Left,
        TopRight = Top | Right,
        BottomRight = Bottom | Right
    };

    Surface *surface() const { return m_surface; }
    ShellView *viewForOutput(Output *o);
    void setWorkspace(AbstractWorkspace *ws);
    Compositor *compositor() const;
    AbstractWorkspace *workspace() const;

    void setConfigureSender(ConfigureSender sender);
    void setToplevel();
    void setTransient(Surface *parent, int x, int y, bool inactive);
    void setPopup(Surface *parent, Seat *seat, int x, int y);
    void setMaximized();
    void setFullscreen();
    void setXWayland(int x, int y, bool inactive);
    void move(Seat *seat) override;
    void resize(Seat *seat, Edges edges);
    void unmap();
    void sendPopupDone();
    void minimize();
    void restore();
    void close();

    void preview(Output *output);
    void endPreview(Output *output);

    void moveViews(double x, double y);

    void setTitle(const QString &title);
    void setAppId(const QString &appid);
    void setGeometry(int x, int y, int w, int h);
    void setPid(pid_t pid);

    Type type() const { return m_type; }
    bool isFullscreen() const;
    bool isInactive() const;
    QRect geometry() const;
    QString title() const;
    QString appId() const;
    Maybe<QPoint> cachedPos() const;
    pid_t pid() const { return m_pid; }

    static ShellSurface *fromSurface(Surface *s);

protected:
    void configure(int x, int y) override;

signals:
    void mapped();
    void contentLost();
    void titleChanged();
    void appIdChanged();
    void popupDone();
    void minimized();
    void restored();

private:
    void parentSurfaceDestroyed();
    QRect surfaceTreeBoundingBox() const;
    void configureToplevel(bool map, bool maximized, bool fullscreen, int dx, int dy);
    void updateState();
    void sendConfigure(int w, int h);
    Output *selectOutput();
    void outputCreated(Output *output);
    void outputRemoved(Output *output);
    void connectParent();
    void disconnectParent();
    inline QString cacheId() const;
    void availableGeometryChanged();
    void workspaceActivated(Workspace *w, Output *o);

    Shell *m_shell;
    Surface *m_surface;
    ConfigureSender m_configureSender;
    AbstractWorkspace *m_workspace;
    QHash<int, ShellView *> m_views;
    QList<ShellView *> m_extraViews;
    ShellView *m_previewView;
    Edges m_resizeEdges;
    bool m_resizing;
    int m_height, m_width;
    QRect m_geometry;
    QRect m_nextGeometry;
    QString m_title;
    QString m_appId;
    bool m_forceMap;
    pid_t m_pid;

    Type m_type;
    Type m_nextType;

    Surface *m_parent;
    QList<QMetaObject::Connection> m_parentConnections;
    struct {
        int x;
        int y;
        Seat *seat;
    } m_popup;
    struct {
        bool maximized;
        bool fullscreen;
        Output *output;
    } m_toplevel;
    struct {
        int x;
        int y;
        bool inactive;
    } m_transient;

    struct {
        QSize size;
        bool maximized;
        bool fullscreen;
    } m_state;

    static QHash<QString, QPoint> s_posCache;

    friend class XWayland;
};

}

DECLARE_OPERATORS_FOR_FLAGS(Orbital::ShellSurface::Type)
DECLARE_OPERATORS_FOR_FLAGS(Orbital::ShellSurface::Edges)

#endif
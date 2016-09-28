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
#include <unordered_map>
#include <memory>

#include <QObject>
#include <QRect>

#include "interface.h"
#include "utils.h"
#include "stringview.h"

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
class PointerGrab;
struct Listener;
class Surface;
class Pointer;

class ShellSurface : public Object
{
    Q_OBJECT
public:
    class Handler {
    public:
        Handler() {}
        template<class T>
        Handler(T &t)
            : m_if(std::make_unique<If<T>>(t))
        {}

        inline void setSize(int w, int h) { m_if->setSize(w, h); }
        inline QRect geometry() const { return m_if->geometry(); }

        inline operator bool() const { return m_if.get(); }

    private:
        struct AbstractIf
        {
            virtual ~AbstractIf() = default;
            virtual void setSize(int w, int h) = 0;
            virtual QRect geometry() const = 0;
        };
        template<class T>
        struct If : AbstractIf
        {
            If(T &t) : data(t) {}
            void setSize(int w, int h) override { data.setSize(w, h); }
            QRect geometry() const override { return data.geometry(); }
            T data;
        };
        std::unique_ptr<AbstractIf> m_if;
    };

    ShellSurface(Shell *shell, Surface *surface, Handler hnd);
    ~ShellSurface();

    void setHandler(Handler hnd);

    enum class Type {
        None = 0,
        Toplevel = 1,
        Transient = 3,
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

    void setToplevel();
    void setParent(Surface *parent, int x, int y, bool inactive);
    void setMaximized();
    void setFullscreen();
    void move(Seat *seat);
    void resize(Seat *seat, Edges edges);
    void unmap();
    void minimize();
    void restore();
    void close();

    void preview(Output *output);
    void endPreview(Output *output);

    void moveViews(double x, double y);

    void setTitle(StringView title);
    void setAppId(StringView appid);
    void setPid(pid_t pid);

    void setIsResponsive(bool responsive);
    bool isResponsive() const { return m_isResponsive; }
    void setBusyCursor(Pointer *pointer);

    Type type() const { return m_type; }
    bool isFullscreen() const;
    bool isInactive() const;
    QRect geometry() const;
    StringView title() const;
    StringView appId() const;
    Maybe<QPoint> cachedPos() const;
    pid_t pid() const { return m_pid; }

    void committed(int x, int y);

signals:
    void mapped();
    void contentLost();
    void titleChanged();
    void appIdChanged();
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
    inline std::string cacheId() const;
    void availableGeometryChanged();
    void workspaceActivated(Workspace *w, Output *o);

    Shell *m_shell;
    Surface *m_surface;
    Handler m_handler;
    AbstractWorkspace *m_workspace;
    std::unordered_map<int, ShellView *> m_views;
    std::vector<ShellView *> m_extraViews;
    ShellView *m_previewView;
    Edges m_resizeEdges;
    bool m_resizing;
    int m_height, m_width;
    QRect m_geometry;
    QRect m_nextGeometry;
    std::string m_title;
    std::string m_appId;
    bool m_forceMap;
    pid_t m_pid;
    PointerGrab *m_currentGrab;
    bool m_isResponsive;

    Type m_type;
    Type m_nextType;

    Surface *m_parent;
    std::vector<QMetaObject::Connection> m_parentConnections;
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

    static std::unordered_map<std::string, QPoint> s_posCache;

    friend class XWayland;
};

DECLARE_OPERATORS_FOR_FLAGS(ShellSurface::Type)
DECLARE_OPERATORS_FOR_FLAGS(ShellSurface::Edges)

}

#endif

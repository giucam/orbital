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

#ifndef ORBITAL_WORKSPACE_H
#define ORBITAL_WORKSPACE_H

#include <unordered_map>

#include <QRect>

#include "interface.h"
#include "transform.h"
#include "animation.h"

struct weston_surface;

namespace Orbital {

class Shell;
class Layer;
class View;
class ShellSurface;
class Output;
class Compositor;
class DummySurface;
class Pager;
class Surface;
class Root;
class Transform;


class AbstractWorkspace
{
protected:
    AbstractWorkspace() : m_mask(0) {}

public:
    class View
    {
    public:
        View(Compositor *c, Output *o);
        virtual ~View();

        virtual void configure(Orbital::View *view) = 0;
        virtual void configureFullscreen(Orbital::View *view, Orbital::View *blackSurface) = 0;
        virtual void setMask(const QRect &mask) {}

        QPointF map(double x, double y) const;
        QPoint pos() const;
        void setPos(double x, double y);
        void setTransformParent(Orbital::View *p);
        void takeView(Orbital::View *p);
        void resetMask();
        inline QRect mask() const { return m_mask; }
        void setTransform(const Transform &tf, bool animate);
        const Transform &transform() const;

    private:
        void updateAnim(double v);

        Root *m_root;
        Output *m_output;
        struct {
            Transform orig, target;
            Animation anim;
        } m_transformAnim;
        QRect m_mask;
    };

    virtual View *viewForOutput(Output *o) = 0;
    virtual void activate(Output *o) = 0;

    int mask() const { return m_mask; }

protected:
    void setMask(int m) { m_mask = m; }

private:
    int m_mask;
};

template<class T>
typename T::View *workspaceViewForOutput(T *ws, Output *o)
{
    AbstractWorkspace::View *v = ws->viewForOutput(o);
    return static_cast<typename T::View *>(v);
}

// Due to mask() being an int, we can have up to 31 workspaces
class Workspace : public Object, public AbstractWorkspace
{
    Q_OBJECT
public:
    class View : public AbstractWorkspace::View
    {
    public:
        View(Workspace *ws, Output *o);
        ~View();

        void configure(Orbital::View *view) override;
        void configureFullscreen(Orbital::View *view, Orbital::View *blackSurface) override;

        void setBackground(Surface *surface);
        QPoint logicalPos() const;

        bool ownsView(Orbital::View *view) const;

        Workspace *workspace() const { return m_workspace; }

    protected:
        void setMask(const QRect &r) override;

    private:
        Workspace *m_workspace;
        Output *m_output;
        Layer *m_backgroundLayer;
        Layer *m_layer;
        Layer *m_fullscreenLayer;
        Orbital::View *m_background;

        friend Pager;
        friend Workspace;
    };

    Workspace(Shell *shell, int id);
    ~Workspace();

    Compositor *compositor() const;
    Pager *pager() const;
    AbstractWorkspace::View *viewForOutput(Output *o) override;
    void activate(Output *o) override;
    Orbital::View *topView() const;

    int id() const;
    int x() const;
    int y() const;
    void setPos(int x, int y);

signals:
    void positionChanged(int x, int y);

private:
    void outputRemoved(Output *o);
    inline void newOutput(Output *o);

    Shell *m_shell;
    int m_id;
    int m_x;
    int m_y;
    std::unordered_map<int, View *> m_views;

    friend Pager;
};

}

#endif

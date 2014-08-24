
#ifndef ORBITAL_SHELLVIEW_H
#define ORBITAL_SHELLVIEW_H

#include <QPoint>

#include "view.h"

namespace Orbital {

class ShellSurface;
class Output;
class Layer;
class WorkspaceView;

class ShellView : public View
{
public:
    explicit ShellView(ShellSurface *shsurf, weston_view *view);
    ~ShellView();

    ShellSurface *surface() const;

    void setDesignedOutput(Output *o);
    void configureToplevel(bool maximized, int dx, int dy);
    void configurePopup(ShellView *parent, int x, int y);

private:
    ShellSurface *m_surface;
    Output *m_designedOutput;
    QPointF m_savedPos;
    bool m_posSaved;
    bool m_maximized;
};

}

#endif

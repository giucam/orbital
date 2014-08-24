
#ifndef ORBITAL_SHELLVIEW_H
#define ORBITAL_SHELLVIEW_H

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
    void configureToplevel();
    void configurePopup(ShellView *parent, int x, int y);

private:
    ShellSurface *m_surface;
    Output *m_designedOutput;
};

}

#endif

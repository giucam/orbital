
#ifndef ORBITAL_VIEW_H
#define ORBITAL_VIEW_H

struct weston_view;

namespace Orbital
{

class Output;
class Layer;
class WorkspaceView;

class View
{
public:
    explicit View(weston_view *view);
    virtual ~View();

    bool isMapped() const;
    void setOutput(Output *o);
    void setPos(int x, int y);
    void setTransformParent(View *p);

    void update();

    Output *output() const;

private:
    weston_view *m_view;
    Output *m_output;

    friend Layer;
    friend WorkspaceView;
};

}

#endif

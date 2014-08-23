
#ifndef ORBITAL_DUMMYSURFACE_H
#define ORBITAL_DUMMYSURFACE_H

#include "view.h"

struct weston_surface;

namespace Orbital {

class Compositor;

class DummySurface : public View
{
public:
    ~DummySurface();

private:
    DummySurface(weston_surface *s, int width, int height);

    weston_surface *m_surface;

    friend Compositor;
};

}

#endif

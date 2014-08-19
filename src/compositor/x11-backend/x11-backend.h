
#ifndef ORBITAL_X11_BACKEND_H
#define ORBITAL_X11_BACKEND_H

#include "backend.h"

namespace Orbital {

class X11Backend : public Backend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Orbital.Compositor.Backend" FILE "x11-backend.json")
    Q_INTERFACES(Orbital::Backend)
public:
    X11Backend();

    bool init(weston_compositor *c) override;
};

}

#endif

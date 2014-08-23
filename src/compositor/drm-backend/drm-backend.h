
#ifndef ORBITAL_DRM_BACKEND_H
#define ORBITAL_DRM_BACKEND_H

#include "backend.h"

namespace Orbital {

class DrmBackend : public Backend
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Orbital.Compositor.Backend" FILE "drm-backend.json")
    Q_INTERFACES(Orbital::Backend)
public:
    DrmBackend();

    bool init(weston_compositor *c) override;
};

}

#endif


#ifndef ORBITAL_WLSHELLSURFACE_H
#define ORBITAL_WLSHELLSURFACE_H

#include "interface.h"

struct wl_resource;

namespace Orbital {

class WlShell;
class ShellSurface;

class WlShellSurface : public Interface
{
    Q_OBJECT
public:
    WlShellSurface(WlShell *shell, ShellSurface *shsurf, wl_client *client, uint32_t id);

private:
    void resourceDestroyed();
    ShellSurface *shellSurface() const;
    void popupDone();

    void pong(uint32_t serial);
    void move(wl_resource *seat_resource, uint32_t serial);
    void resize(wl_resource *seat_resource, uint32_t serial, uint32_t edges);
    void setToplevel();
    void setTransient(wl_resource *parent_resource, int x, int y, uint32_t flags);
    void setFullscreen(uint32_t method, uint32_t framerate, wl_resource *output_resource);
    void setPopup(wl_resource *seat_resource, uint32_t serial, wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags);
    void setMaximized(wl_resource *output_resource);
    void setTitle(const char *title);
    void setClass(const char *className);

    WlShell *m_wlShell;
    wl_resource *m_resource;
};

}

#endif

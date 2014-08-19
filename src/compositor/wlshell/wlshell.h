
#ifndef ORBITAL_WLSHELL_H
#define ORBITAL_WLSHELL_H

#include "interface.h"

struct wl_resource;

namespace Orbital {

class Shell;
class Compositor;
class ShellSurface;

class WlShell : public Interface, public Global
{
    Q_OBJECT
public:
    explicit WlShell(Shell *shell, Compositor *c);

protected:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

private:
    ShellSurface *getShellSurface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource);
//     void pointerFocus(ShellSeat *seat, weston_pointer *pointer);
//     void surfaceResponsiveness(WlShellSurface *shsurf);

//     static void sendConfigure(weston_surface *surface, int32_t width, int32_t height);

    Shell *m_shell;

    static const struct wl_shell_interface shell_implementation;
    static const struct weston_shell_client shell_client;
};

}

#endif
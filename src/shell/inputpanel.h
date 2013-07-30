
#ifndef INPUTPANEL_H
#define INPUTPANEL_H

#include <wayland-client.h>

struct wl_client;
struct wl_resource;
struct wl_input_panel_surface_interface;
struct wl_input_panel_interface;

class Shell;

class InputPanel {
public:
    InputPanel(wl_display *display);

private:
    static void bind(wl_client *client, void *data, uint32_t version, uint32_t id);
    static void input_panel_surface_set_toplevel(wl_client *client, wl_resource *resource, wl_resource *output_resource, uint32_t position);
    static void input_panel_surface_set_overlay_panel(wl_client *client, wl_resource *resource);
    static void input_panel_get_input_panel_surface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource);

    static const wl_input_panel_surface_interface input_panel_surface_implementation;
    static const wl_input_panel_interface input_panel_implementation;
};

#endif


#include <stdio.h>

#include <wayland-server.h>

#include "inputpanel.h"
#include "wayland-input-method-server-protocol.h"

InputPanel::InputPanel(wl_display *display)
{
    wl_global_create(display, &wl_input_panel_interface, 1, this, bind);
}

void InputPanel::bind(wl_client *client, void *data, uint32_t version, uint32_t id)
{
    wl_resource *resource = wl_resource_create(client, &wl_input_panel_interface, 1, id);

//     if (shell->input_panel.binding == NULL) {
        wl_resource_set_implementation(resource, &input_panel_implementation, data, [](wl_resource *resource){});
        return;
//     }

    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "interface object already bound");
    wl_resource_destroy(resource);
}

void InputPanel::input_panel_surface_set_toplevel(wl_client *client, wl_resource *resource,  wl_resource *output_resource, uint32_t position)
{
}

void InputPanel::input_panel_surface_set_overlay_panel( wl_client *client, wl_resource *resource)
{
}

const struct wl_input_panel_surface_interface InputPanel::input_panel_surface_implementation = {
    input_panel_surface_set_toplevel,
    input_panel_surface_set_overlay_panel
};

void InputPanel::input_panel_get_input_panel_surface(wl_client *client, wl_resource *resource, uint32_t id, wl_resource *surface_resource)
{
//     struct weston_surface *surface = wl_resource_get_user_data(surface_resource);
//     struct desktop_shell *shell = wl_resource_get_user_data(resource);
//     struct input_panel_surface *ipsurf;

//     if (get_input_panel_surface(surface)) {
//         wl_resource_post_error(surface_resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "wl_input_panel::get_input_panel_surface already requested");
//         return;
//     }
//
//     ipsurf = create_input_panel_surface(shell, surface);
//     if (!ipsurf) {
//         wl_resource_post_error(surface_resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "surface->configure already set");
//         return;
//     }

//     ipsurf->resource =
    wl_resource *res = wl_resource_create(client, &wl_input_panel_surface_interface, 1, id);
    wl_resource_set_implementation(res, &input_panel_surface_implementation, 0, [](wl_resource *resource){});
}

const struct wl_input_panel_interface InputPanel::input_panel_implementation = {
    input_panel_get_input_panel_surface
};
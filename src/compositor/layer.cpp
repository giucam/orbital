
#include <weston/compositor.h>

#include "layer.h"
#include "view.h"

namespace Orbital {

Layer::Layer(weston_layer *l)
     : m_layer(l)
{
}

Layer::Layer(Layer *p)
     : m_layer(new weston_layer)
{
    weston_layer_init(m_layer, nullptr);
    wl_list_init(&m_layer->link);

    if (p) {
        append(p);
    }
}

void Layer::append(Layer *l)
{
    wl_list_remove(&m_layer->link);
    wl_list_insert(&l->m_layer->link, &m_layer->link);
}

void Layer::addView(View *view)
{
    if (view->m_view->layer_link.link.prev) {
        weston_layer_entry_remove(&view->m_view->layer_link);
    }
    weston_layer_entry_insert(&m_layer->view_list, &view->m_view->layer_link);
}

void Layer::setMask(int x, int y, int w, int h)
{
    weston_layer_set_mask(m_layer, x, y, w, h);
}

}

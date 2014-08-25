/*
 * Copyright 2013-2014 Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This file is part of Orbital
 *
 * Orbital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orbital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orbital.  If not, see <http://www.gnu.org/licenses/>.
 */

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

/*
 * Copyright 2013  Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "layer.h"
#include "shellsurface.h"

Layer::Layer()
     : m_below(nullptr)
{

}

void Layer::init()
{
    weston_layer_init(&m_layer, nullptr);
}

void Layer::init(struct weston_layer *below)
{
    weston_layer_init(&m_layer, &below->link);
}

void Layer::init(Layer *below)
{
    init(&below->m_layer);
}

void Layer::hide()
{
    m_below = m_layer.link.prev;
    wl_list_remove(&m_layer.link);
}

void Layer::show()
{
    if (m_below) {
        wl_list_insert(m_below, &m_layer.link);
    }
}

void Layer::addSurface(struct weston_surface *surf)
{
    wl_list_insert(&m_layer.surface_list, &surf->layer_link);
}

void Layer::addSurface(ShellSurface *surf)
{
    addSurface(surf->m_surface);
}

void Layer::restack(struct weston_surface *surf)
{
    weston_surface_restack(surf, &m_layer.surface_list);
}

void Layer::restack(ShellSurface *surf)
{
    restack(surf->m_surface);
}

Layer::Iterator Layer::begin()
{
    printf("begin %p %p\n",&m_layer.surface_list,m_layer.surface_list.next);
    return Iterator(&m_layer.surface_list, m_layer.surface_list.next);
}

Layer::Iterator Layer::end()
{
//     return Iterator(container_of(&m_layer.surface_list, struct weston_surface, layer_link));
    return Iterator(&m_layer.surface_list, &m_layer.surface_list);
}

Layer::Iterator::Iterator(const Iterator &it)
               : m_list(it.m_list)
               , m_elm(it.m_elm)
               , m_next(it.m_next)
{

}

Layer::Iterator::Iterator(struct wl_list *list, struct wl_list *elm)
               : m_list(list)
               , m_elm(elm)
{
    m_next = m_elm->next;
}

Layer::Iterator &Layer::Iterator::operator=(const Iterator &it)
{
    m_list = it.m_list;
    m_elm = it.m_elm;
    m_next = it.m_next;
    return *this;
}

bool Layer::Iterator::operator==(const Iterator &it)
{
    return m_elm == it.m_elm;
}

bool Layer::Iterator::operator!=(const Iterator &it)
{
    return m_elm != it.m_elm;
}

struct weston_surface *Layer::Iterator::operator*() const
{
    return deref();
}

struct weston_surface *Layer::Iterator::operator->() const
{
    return deref();
}

Layer::Iterator &Layer::Iterator::operator++()
{
    if (m_list != m_elm) {
        m_elm = m_next;
        m_next = m_elm->next;
    }
    return *this;
}

struct weston_surface *Layer::Iterator::deref() const
{
    if (m_elm) {
        return container_of(m_elm, struct weston_surface, layer_link);
    } else {
        return nullptr;
    }
}

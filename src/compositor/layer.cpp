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

#include <assert.h>

#include <QDebug>

#include <compositor.h>

#include "layer.h"
#include "view.h"

namespace Orbital {

struct Wrapper {
    weston_layer layer;
    Layer *parent;
};

Layer::Layer(weston_layer *l)
     : m_layer(std::make_unique<Wrapper>())
     , m_parent(nullptr)
     , m_acceptInput(true)
{
    m_layer->parent = this;

    weston_layer_init(&m_layer->layer, nullptr);
    wl_list_init(&m_layer->layer.link);
    wl_list_insert(&l->link, &m_layer->layer.link);
}

Layer::Layer(Layer *parent)
     : m_layer(std::make_unique<Wrapper>())
     , m_parent(parent)
     , m_acceptInput(true)
{
    m_layer->parent = this;

    weston_layer_init(&m_layer->layer, nullptr);
    wl_list_init(&m_layer->layer.link);

    if (parent) {
        parent->addChild(this);
    }
}

Layer::Layer(Layer &&l)
    : m_layer(std::move(l.m_layer))
    , m_parent(l.m_parent)
    , m_acceptInput(l.m_acceptInput)
{
    m_layer->parent = this;

    if (m_parent) {
        auto it = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), &l);
        assert(it != m_parent->m_children.end());
        *it = this;
    }

    m_children = l.m_children;
    for (Layer *c: m_children) {
        c->m_parent = this;
    }

    l.m_parent = nullptr;
    l.m_children.clear();
    l.m_layer = nullptr;
}

Layer::~Layer()
{
    if (m_parent) {
        m_parent->removeChild(this);
    }

    for (Layer *c: m_children) {
        c->m_parent = nullptr;
        wl_list_remove(&c->m_layer->layer.link);
        wl_list_init(&c->m_layer->layer.link);
    }
    if (m_layer) {
        wl_list_remove(&m_layer->layer.link);
    }
}

void Layer::setParent(Layer *parent)
{
    if (m_parent) {
        m_parent->removeChild(this);
    }
    wl_list_remove(&m_layer->layer.link);

    wl_list_init(&m_layer->layer.link);
    m_parent = parent;
    if (m_parent) {
        m_parent->addChild(this);
    }
}

void Layer::addChild(Layer *l)
{
    int n = m_children.size();
    Layer *p = n ? m_children[n - 1] : this;

    wl_list_insert(&p->m_layer->layer.link, &l->m_layer->layer.link);
    m_children.push_back(l);
}

void Layer::removeChild(Layer *l)
{
    auto it = std::find(m_children.begin(), m_children.end(), l);
    m_children.erase(it);
}

void Layer::addView(View *view)
{
    if (view->m_view->layer_link.link.prev) {
        weston_layer_entry_remove(&view->m_view->layer_link);
    }
    weston_layer_entry_insert(&m_layer->layer.view_list, &view->m_view->layer_link);
    view->m_layer = this;
    view->map();
}

void Layer::raiseOnTop(View *view)
{
    weston_layer_entry_remove(&view->m_view->layer_link);
    weston_layer_entry_insert(&m_layer->layer.view_list, &view->m_view->layer_link);
    weston_view_damage_below(view->m_view);
    view->m_layer = this;
}

void Layer::lower(View *view)
{
    weston_layer_entry *next;
    if (wl_list_empty(&view->m_view->layer_link.link)) {
        next = &m_layer->layer.view_list;
    } else {
        next = wl_container_of(view->m_view->layer_link.link.next, (weston_layer_entry *)nullptr, link);
    }
    weston_layer_entry_remove(&view->m_view->layer_link);

    weston_layer_entry_insert(next, &view->m_view->layer_link);
    weston_view_damage_below(view->m_view);
    view->m_layer = this;
}

View *Layer::topView() const
{
    if (wl_list_empty(&m_layer->layer.view_list.link)) {
        return nullptr;
    }

    weston_view *v = wl_container_of(m_layer->layer.view_list.link.next, (weston_view *)nullptr, layer_link.link);
    return View::fromView(v);
}

void Layer::setMask(int x, int y, int w, int h)
{
    weston_layer_set_mask(&m_layer->layer, x, y, w, h);
}

void Layer::setAcceptInput(bool accept)
{
    m_acceptInput = accept;
}

Layer *Layer::fromLayer(weston_layer *l)
{
    return reinterpret_cast<Wrapper *>(l)->parent;
}

}

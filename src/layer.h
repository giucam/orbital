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

#ifndef LAYER_H
#define LAYER_H

#include <weston/compositor.h>

class ShellSurface;

class Layer {
public:
    class Iterator {
    public:
        Iterator(const Iterator &it);

        Iterator &operator=(const Iterator &it);

        bool operator==(const Iterator &it);
        bool operator!=(const Iterator &it);

        struct weston_surface *operator*() const;
        struct weston_surface *operator->() const;

        Iterator &operator++();

    private:
        Iterator(struct wl_list *list, struct wl_list *elm);
        struct weston_surface *deref() const;

        struct wl_list *m_list;
        struct wl_list *m_elm;
        // this m_next is needed to do what wl_list_for_each_safe does, that is
        // it allows for the current element to be removed from the list
        // without having the iterator go berserk.
        struct wl_list *m_next;

        friend class Layer;
    };

    Layer();

    void init();
    void init(struct weston_layer *below);
    void init(Layer *below);
    void hide();
    void show();

    void addSurface(struct weston_surface *surf);
    void addSurface(ShellSurface *surf);
    void restack(struct weston_surface *surf);
    void restack(ShellSurface *surf);

    Iterator begin();
    Iterator end();

private:
    struct weston_layer m_layer;
    struct wl_list *m_below;
};

#endif

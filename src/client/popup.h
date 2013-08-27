/*
 * Copyright 2013 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef POPUP_H
#define POPUP_H

#include <QObject>

class QQuickItem;
class QQuickWindow;

struct desktop_shell_surface;
struct desktop_shell_surface_listener;

class Popup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *parentItem READ parentItem WRITE setParentItem)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QQuickItem *content READ content WRITE setContent)
public:
    Popup(QObject *parent = nullptr);
    ~Popup();

    QQuickItem *parentItem() const { return m_parent; }
    void setParentItem(QQuickItem *i) { m_parent = i; }

    bool visible() const;
    void setVisible(bool v);

    QQuickItem *content() const { return m_content; }
    void setContent(QQuickItem *c) { m_content = c; }

public slots:
    void show();
    void hide();

signals:
    void visibleChanged();

private slots:
    void hideEvent();

private:
    void close(desktop_shell_surface *s);

    QQuickWindow *m_window;
    QQuickItem *m_parent;
    QQuickItem *m_content;
    desktop_shell_surface *m_shsurf;

    static const desktop_shell_surface_listener m_shsurf_listener;
};

#endif

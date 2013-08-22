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

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QQuickItem>

class QQuickWindow;
class QTimer;

class ToolTip : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *content READ content WRITE setContent)
public:
    ToolTip(QQuickItem *parent = nullptr);

    QQuickItem *content() const { return m_content; }
    void setContent(QQuickItem *c) { m_content = c; }

public slots:
    void show();
    void hide();

private:
    void doShow();

    QQuickWindow *m_window;
    QQuickItem *m_content;
    QTimer *m_showTimer;
    QTimer *m_hideTimer;

    static int s_showing;
};

#endif

/*
 * Copyright 2014 Giulio Camuffo <giuliocamuffo@gmail.com>
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

#ifndef ORBITAL_LAUNCHER_MATCHER_MODEL_H
#define ORBITAL_LAUNCHER_MATCHER_MODEL_H

#include <QAbstractListModel>
#include <QLinkedList>

class MatcherModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString expression READ expression WRITE setExpression)
public:
    MatcherModel();

    QString expression() const;
    void setExpression(const QString &expr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void matchExpression();

    QString m_expression;
    QStringList m_items;
    QStringList m_matches;
};

#endif

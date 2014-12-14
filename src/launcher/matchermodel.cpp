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

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include <QDebug>
#include <QDir>

#include "matchermodel.h"

MatcherModel::MatcherModel()
            : QAbstractListModel()
{
    QString path = qgetenv("PATH");
    for (const QString &p: path.split(':')) {
        QDir dir(p);
        for (const QFileInfo &f: dir.entryInfoList(QDir::Files)) {
            if (!f.isExecutable()) {
                continue;
            }

            m_items << f.fileName();
        }
    }
    m_items.sort();
    m_items.removeDuplicates();

    matchExpression();
}

QString MatcherModel::expression() const
{
    return m_expression;
}

void MatcherModel::setExpression(const QString &expr)
{
    QString e = expr.split(' ').first();
    if (m_expression == e) {
        return;
    }

    m_expression = e;
    matchExpression();
}

int MatcherModel::rowCount(const QModelIndex &parent) const
{
    return m_matches.count();
}

QVariant MatcherModel::data(const QModelIndex &index, int role) const
{
    return m_matches.at(index.row());
}

void MatcherModel::matchExpression()
{
    beginResetModel();
    m_matches.clear();
    endResetModel();

    QStringList matches;
    for (const QString &entry: m_items) {
        if (entry.startsWith(m_expression)) {
            matches.append(entry);
        }
    }
    beginInsertRows(QModelIndex(), 0, matches.count());
    m_matches = matches;
    endInsertRows();
}

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
#include <QFileSystemWatcher>

#include "matchermodel.h"

MatcherModel::MatcherModel()
            : QAbstractListModel()
            , m_watcher(new QFileSystemWatcher(this))
{
    QString path = QString::fromUtf8(qgetenv("PATH"));
    foreach (const QString &p, path.split(QLatin1Char(':'))) {
        m_watcher->addPath(p);
    }

    buildItemsList();
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &MatcherModel::buildItemsList);
}

void MatcherModel::buildItemsList()
{
    m_items.clear();
    foreach (const QString &p, m_watcher->directories()) {
        QDir dir(p);
        foreach (const QFileInfo &f, dir.entryInfoList(QDir::Files)) {
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
    QString e = expr.split(QLatin1Char(' ')).first();
    if (m_expression == e) {
        return;
    }

    m_expression = e;
    matchExpression();
}

void MatcherModel::setCommandPrefix(const QString &prefix)
{
    m_commandPrefix = prefix;
}

void MatcherModel::addCommand(const QString &command)
{
    m_commands << command;
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

    if (m_expression == m_commandPrefix) {
        matches = m_commands;
    } else if (m_expression.startsWith(m_commandPrefix)) {
        QString command = m_expression.mid(m_commandPrefix.length());
        foreach (const QString &entry, m_commands) {
            if (entry == command) {
                matches.prepend(entry);
            } else if (entry.startsWith(command)) {
                matches.append(entry);
            }
        }
    } else {
        foreach (const QString &entry, m_items) {
            if (entry == m_expression) {
                matches.prepend(entry);
            } else if (entry.startsWith(m_expression)) {
                matches.append(entry);
            }
        }
    }
    beginInsertRows(QModelIndex(), 0, matches.count());
    m_matches = matches;
    endInsertRows();
}

void MatcherModel::addInHistory(const QString &command)
{
    if (!command.isEmpty()) {
        m_items.removeOne(command);
        m_items.prepend(command);
    }
}

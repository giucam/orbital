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

#include <QProcess>

#include "processlauncher.h"

ProcessLauncher::ProcessLauncher(QObject *parent)
               : QObject(parent)
{
}

void ProcessLauncher::launch(const QString &process)
{
    QProcess::startDetached(process);
}

QString ProcessLauncher::run(const QString &process)
{
    QProcess::startDetached(process);
}

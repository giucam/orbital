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

#include <QtQml>

#include "processlauncher.h"

void ProcessLauncherPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<ProcessLauncher>(uri, 1, 0, "ProcessLauncher", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new ProcessLauncher);
    });
    qmlRegisterType<Process>(uri, 1, 0, "Process");
}

ProcessLauncher::ProcessLauncher(QObject *p)
               : QObject(p)
{
}

void ProcessLauncher::launch(const QString &process)
{
    unsetenv("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT");
    QProcess::startDetached(process);
    setenv("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT", "1", 1);
}


Process::Process(QObject *p)
       : QObject(p)
       , m_state(State::NotRunning)
{
    connect(&m_process, (void (QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished, this, &Process::finished);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &Process::readyReadStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &Process::readyReadStandardError);
    connect(&m_process, &QProcess::stateChanged, [this](QProcess::ProcessState st) {
        m_state = (State)st;
        emit stateChanged();
    });
}

void Process::start(const QString &command)
{
    if (m_process.state() == QProcess::NotRunning) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove(QLatin1String("QT_WAYLAND_USE_BYPASSWINDOWMANAGERHINT"));

        m_process.setProcessEnvironment(env);
        m_process.start(command);
    }
}

QByteArray Process::readAllStandardOutput()
{
    return m_process.readAllStandardOutput();
}

QByteArray Process::readAllStandardError()
{
    return m_process.readAllStandardError();
}

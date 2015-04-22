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

#ifndef PROCESSLAUNCHER_H
#define PROCESSLAUNCHER_H

#include <QProcess>
#include <QQmlExtensionPlugin>

class ProcessLauncherPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

class ProcessLauncher : public QObject
{
    Q_OBJECT
public:
    ProcessLauncher(QObject *p = nullptr);

public slots:
    void launch(const QString &process);

};

class Process : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
    enum class State {
        NotRunning = 0,
        Starting = 1,
        Running = 2
    };
    Q_ENUMS(State);

    Process(QObject *p = nullptr);

    State state() const { return m_state; }

    Q_INVOKABLE void start(const QString &process);
    Q_INVOKABLE QByteArray readAllStandardOutput();
    Q_INVOKABLE QByteArray readAllStandardError();

signals:
    void finished();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void stateChanged();

private:
    QProcess m_process;
    State m_state;
};

#endif

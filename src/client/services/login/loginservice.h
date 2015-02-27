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

#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include <QQmlExtensionPlugin>

class QJSValue;

class PamAuthenticator;

class LoginPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};

class LoginManagerBackend : public QObject
{
    Q_OBJECT
public:
    virtual ~LoginManagerBackend() {}

    virtual void poweroff() = 0;
    virtual void reboot() = 0;
    virtual void locked() {}
    virtual void unlocked() {}

signals:
    void requestLock();
    void requestUnlock();
};

class LoginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
public:
    enum class Result {
        AuthenticationSucceded,
        AuthenticationFailed,
        Error
    };
    Q_ENUMS(Result)

    LoginManager(QObject *p = nullptr);
    ~LoginManager();

    bool busy() const;

public slots:
    void abort();
    void logOut();
    void poweroff();
    void reboot();
    void requestLogOut();
    void requestPoweroff();
    void requestReboot();
    void requestHandled();
    void lockSession();
    void unlockSession();
    void tryUnlockSession(const QString &password, const QJSValue &callback);

signals:
    void logOutRequested();
    void poweroffRequested();
    void rebootRequested();
    void aborted();
    void busyChanged();

private slots:
    void doRequest();

private:
    LoginManagerBackend *m_backend;
    void (LoginManager::*m_request)();
    bool m_requestHandled;
    PamAuthenticator *m_authenticator;
    QThread *m_authenticatorThread;
    bool m_busy;
};

#endif

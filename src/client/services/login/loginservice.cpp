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

#include <unistd.h>
#include <pwd.h>
#include <security/pam_appl.h>

#include <QTimer>
#include <QThread>
#include <QJSValue>
#include <QDebug>
#include <QtQml>

#include "loginservice.h"
#include "client.h"
#include "clibackend.h"
#ifdef USE_LOGIND
#include "logindbackend.h"
#endif

Q_DECLARE_METATYPE(LoginManager::Result)

void LoginPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<LoginManager>(uri, 1, 0, "LoginManager", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new LoginManager);
    });
}


class PamAuthenticator : public QObject
{
    Q_OBJECT
public:
    PamAuthenticator()
        : m_reply(nullptr)
    {
    }

    ~PamAuthenticator()
    {
        delete m_reply;
    }

    Q_INVOKABLE void authenticateUser(const QString &password)
    {
        const pam_conv local_conversation = { function_conversation, this };
        pam_handle_t *local_auth_handle = nullptr; // this gets set by pam_start

        passwd *pwd = getpwuid(getuid());
        if (!pwd) {
            emit authenticationResult(LoginManager::Result::Error);
            return;
        }

        int retval = pam_start("su", pwd->pw_name, &local_conversation, &local_auth_handle);
        if (retval != PAM_SUCCESS) {
                qWarning("pam_start returned %d", retval);
                emit authenticationResult(LoginManager::Result::Error);
                return;
        }

        m_reply = new pam_response;

        m_reply[0].resp = strdup(qPrintable(password));
        m_reply[0].resp_retcode = 0;

        retval = pam_authenticate(local_auth_handle, 0);
        if (retval != PAM_SUCCESS) {
            if (retval == PAM_AUTH_ERR) {
                emit authenticationResult(LoginManager::Result::AuthenticationFailed);
                return;
            } else {
                qWarning("pam_authenticate returned %d", retval);
                emit authenticationResult(LoginManager::Result::Error);
                return;
            }
        }

        retval = pam_end(local_auth_handle, retval);
        if (retval != PAM_SUCCESS) {
            qWarning("pam_end returned %d", retval);
            emit authenticationResult(LoginManager::Result::Error);
            return;
        }

        emit authenticationResult(LoginManager::Result::AuthenticationSucceded);
    }

signals:
    void authenticationResult(LoginManager::Result res);

private:
    static int function_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp,
                              void *appdata_ptr)
    {
        PamAuthenticator *_this = static_cast<PamAuthenticator *>(appdata_ptr);
        *resp = _this->m_reply;
        return PAM_SUCCESS;
    }

    pam_response *m_reply;
};

LoginManager::LoginManager(QObject *p)
            : QObject(p)
            , m_backend(nullptr)
            , m_authenticator(new PamAuthenticator)
            , m_authenticatorThread(new QThread)
            , m_busy(false)
{
    qRegisterMetaType<Result>();

#ifdef USE_LOGIND
    m_backend = LogindBackend::create();
#endif
    if (!m_backend) {
        m_backend = CliBackend::create();
    }

    if (m_backend) {
        connect(m_backend, &LoginManagerBackend::requestLock, this, &LoginManager::lockSession);
        connect(m_backend, &LoginManagerBackend::requestUnlock, this, &LoginManager::unlockSession);
        connect(Client::client(), &Client::locked, m_backend, &LoginManagerBackend::locked);
        connect(Client::client(), &Client::unlocked, m_backend, &LoginManagerBackend::unlocked);
        connect(Client::client(), &Client::locked, this, &LoginManager::sessionLocked);
        connect(Client::client(), &Client::unlocked, this, &LoginManager::sessionUnlocked);
    }

    m_authenticator->moveToThread(m_authenticatorThread);
    m_authenticatorThread->start();
}

LoginManager::~LoginManager()
{
    delete m_backend;
    delete m_authenticator;
}

bool LoginManager::busy() const
{
    return m_busy;
}

void LoginManager::abort()
{
    m_request = nullptr;
    emit aborted();
}

void LoginManager::logOut()
{
    Client::client()->quit();
}

void LoginManager::poweroff()
{
    Client::client()->quit();
    if (m_backend) {
        m_backend->poweroff();
    }
}

void LoginManager::reboot()
{
    Client::client()->quit();
    if (m_backend) {
        m_backend->reboot();
    }
}

void LoginManager::requestLogOut()
{
    m_request = &LoginManager::logOut;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit logOutRequested();
}

void LoginManager::requestPoweroff()
{
    m_request = &LoginManager::poweroff;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit poweroffRequested();
}

void LoginManager::requestReboot()
{
    m_request = &LoginManager::reboot;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit rebootRequested();
}

void LoginManager::requestHandled()
{
    m_requestHandled = true;
}

void LoginManager::lockSession()
{
    Client::client()->lockSession();
}

void LoginManager::unlockSession()
{
    Client::client()->unlockSession();
}

void LoginManager::tryUnlockSession(const QString &password, const QJSValue &callback)
{
    if (m_busy) {
        return;
    }

    class Authenticator : public QObject
    {
    public:
        Authenticator(LoginManager *s, const QJSValue &cb)
            : service(s)
            , callback(cb)
        {
            connect(service->m_authenticator, &PamAuthenticator::authenticationResult, this, &Authenticator::result);
        }
        void result(LoginManager::Result r)
        {
            if (callback.isCallable()) {
                callback.call(QJSValueList() << (int)r);
            }
            service->m_busy = false;
            emit service->busyChanged();
            if (r == LoginManager::Result::AuthenticationSucceded) {
                service->unlockSession();
            }

            deleteLater();
        }

        LoginManager *service;
        QJSValue callback;
    };

    new Authenticator(this, callback);
    QMetaObject::invokeMethod(m_authenticator, "authenticateUser", Q_ARG(QString, password));
    m_busy = true;
    emit busyChanged();
}

void LoginManager::doRequest()
{
    if (!m_requestHandled && m_request) {
        (this->*m_request)();
    }
}

#include "loginservice.moc"

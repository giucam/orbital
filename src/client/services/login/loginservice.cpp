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

#include "loginservice.h"
#include "client.h"
#include "clibackend.h"
#ifdef USE_LOGIND
#include "logindbackend.h"
#endif

Q_DECLARE_METATYPE(LoginService::Result)

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
            emit authenticationResult(LoginService::Result::Error);
            return;
        }

        int retval = pam_start("su", pwd->pw_name, &local_conversation, &local_auth_handle);
        if (retval != PAM_SUCCESS) {
                qWarning("pam_start returned %d", retval);
                emit authenticationResult(LoginService::Result::Error);
                return;
        }

        m_reply = new pam_response;

        m_reply[0].resp = strdup(qPrintable(password));
        m_reply[0].resp_retcode = 0;

        retval = pam_authenticate(local_auth_handle, 0);
        if (retval != PAM_SUCCESS) {
            if (retval == PAM_AUTH_ERR) {
                emit authenticationResult(LoginService::Result::AuthenticationFailed);
                return;
            } else {
                qWarning("pam_authenticate returned %d", retval);
                emit authenticationResult(LoginService::Result::Error);
                return;
            }
        }

        retval = pam_end(local_auth_handle, retval);
        if (retval != PAM_SUCCESS) {
            qWarning("pam_end returned %d", retval);
            emit authenticationResult(LoginService::Result::Error);
            return;
        }

        emit authenticationResult(LoginService::Result::AuthenticationSucceded);
    }

signals:
    void authenticationResult(LoginService::Result res);

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

LoginService::LoginService()
            : Service()
            , m_backend(nullptr)
            , m_authenticator(new PamAuthenticator)
            , m_authenticatorThread(new QThread)
            , m_busy(false)
{
    qRegisterMetaType<Result>();

    m_authenticator->moveToThread(m_authenticatorThread);
    m_authenticatorThread->start();
}

LoginService::~LoginService()
{
    delete m_backend;
    delete m_authenticator;
}

void LoginService::init()
{
#ifdef USE_LOGIND
    m_backend = LogindBackend::create();
#endif
    if (!m_backend) {
        m_backend = CliBackend::create();
    }

    if (m_backend) {
        connect(m_backend, &LoginServiceBackend::requestLock, this, &LoginService::lockSession);
        connect(m_backend, &LoginServiceBackend::requestUnlock, this, &LoginService::unlockSession);
    }
}

bool LoginService::busy() const
{
    return m_busy;
}

void LoginService::abort()
{
    m_request = nullptr;
    emit aborted();
}

void LoginService::logOut()
{
    client()->quit();
}

void LoginService::poweroff()
{
    client()->quit();
    if (m_backend) {
        m_backend->poweroff();
    }
}

void LoginService::reboot()
{
    client()->quit();
    if (m_backend) {
        m_backend->reboot();
    }
}

void LoginService::requestLogOut()
{
    m_request = &LoginService::logOut;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit logOutRequested();
}

void LoginService::requestPoweroff()
{
    m_request = &LoginService::poweroff;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit poweroffRequested();
}

void LoginService::requestReboot()
{
    m_request = &LoginService::reboot;
    m_requestHandled = false;
    QTimer::singleShot(200, this, SLOT(doRequest()));
    emit rebootRequested();
}

void LoginService::requestHandled()
{
    m_requestHandled = true;
}

void LoginService::lockSession()
{
    client()->lockSession();
}

void LoginService::unlockSession()
{
    client()->unlockSession();
}

void LoginService::tryUnlockSession(const QString &password, const QJSValue &callback)
{
    if (m_busy) {
        return;
    }

    class Authenticator : public QObject
    {
    public:
        Authenticator(LoginService *s, const QJSValue &cb)
            : service(s)
            , callback(cb)
        {
            connect(service->m_authenticator, &PamAuthenticator::authenticationResult, this, &Authenticator::result);
        }
        void result(LoginService::Result r)
        {
            if (callback.isCallable()) {
                callback.call(QJSValueList() << (int)r);
            }
            service->m_busy = false;
            emit service->busyChanged();
            if (r == LoginService::Result::AuthenticationSucceded) {
                service->unlockSession();
            }

            deleteLater();
        }

        LoginService *service;
        QJSValue callback;
    };

    new Authenticator(this, callback);
    QMetaObject::invokeMethod(m_authenticator, "authenticateUser", Q_ARG(QString, password));
    m_busy = true;
    emit busyChanged();
}

void LoginService::doRequest()
{
    if (!m_requestHandled && m_request) {
        (this->*m_request)();
    }
}

#include "loginservice.moc"

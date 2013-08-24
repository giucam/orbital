
#include <QDBusInterface>

#include "loginservice.h"
#include "client.h"

LoginService::LoginService()
            : Service()
{
}

LoginService::~LoginService()
{

}

void LoginService::init()
{
    m_interface = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                     "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
}

void LoginService::logOut()
{
    client()->quit();
}

void LoginService::poweroff()
{
    client()->quit();
    m_interface->call("PowerOff", true);
}

void LoginService::reboot()
{
    client()->quit();
    m_interface->call("Reboot", true);
}

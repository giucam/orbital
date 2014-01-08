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

#include <QProcess>

#include "clibackend.h"

//TODO: signal new and removed devices and signal a device has been (u)mounted

CliDevice::CliDevice(const QString &udi)
           : Device(udi)
{

}

bool CliDevice::umount()
{
    if (type() != Type::Storage) {
        return false;
    }

    //TODO
    return false;
}

bool CliDevice::mount()
{
    if (type() != Type::Storage) {
        return false;
    }

    //TODO
    return false;
}

static QString run(const QString &cmd)
{
    QProcess proc;
    proc.start(QString("/bin/sh -c \"%1\"").arg(cmd));
    proc.waitForFinished();
    return proc.readAllStandardOutput();
}

bool CliDevice::isMounted() const
{
    if (type() != Type::Storage) {
        return false;
    }

    QString out = run(QString("cat /proc/mounts | grep %1").arg(udi()));
    return !out.isEmpty();
}



CliBackend::CliBackend(HardwareService *hw)
            : HardwareService::Backend(hw)
{
}

CliBackend *CliBackend::create(HardwareService *hw)
{
    CliBackend *cli = new CliBackend(hw);
    if (!cli) {
        return nullptr;
    }

    QString out = run("cat /proc/partitions | grep -iE '[0-9]+ +[a-z0-9:_-]+' | grep -oiE '[a-z]+[0-9:_-]$'");
    QStringList list = out.split("\n");
    list.removeLast();
    for (const QString &c: list) {
        Device *d = new CliDevice(c);
        d->setName(c);
        d->setType(Device::Type::Storage);
        cli->deviceAdded(d);
    }

    return cli;
}

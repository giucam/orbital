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
#include <QDebug>

#include "clibackend.h"

//TODO: signal new and removed devices and signal a device has been (u)mounted

static int run(const QString &cmd, QByteArray *out = nullptr)
{
    QProcess proc;
    QString c = cmd;
    c.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    proc.start(QStringLiteral("/bin/sh"), QStringList() << QStringLiteral("-c") << c);
    proc.waitForFinished();
    if (out) {
        *out = proc.readAllStandardOutput();
    }
    return proc.exitCode();
}

static QProcess *start(const QString &cmd)
{
    QProcess *proc = new QProcess;
    QString c = cmd;
    c.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    proc->start(QStringLiteral("/bin/sh"), QStringList() << QStringLiteral("-c") << c);
    return proc;
}

CliDevice::CliDevice(const QString &udi)
           : Device(udi)
{

}

bool CliDevice::umount()
{
    if (type() != Type::Storage) {
        return false;
    }

    QProcess *proc = start(QStringLiteral("udisksctl unmount -b %1").arg(udi()));
//     QProcess *proc = start(QString("umount %1").arg(udi()));
    QObject::connect(proc, (void (QProcess::*)(int))&QProcess::finished, [this, proc](int exitCode) {
        if (exitCode == 0) {
            emit this->mountedChanged();
        }
        delete proc;
    });

    return true;
}

bool CliDevice::mount()
{
    if (type() != Type::Storage) {
        return false;
    }

    QProcess *proc = start(QStringLiteral("udisksctl mount -b %1").arg(udi()));
//     QProcess *proc = start(QString("mkdir ~/`basename %1` && mount %1 ~/`basename %1`").arg(udi()));
    QObject::connect(proc, (void (QProcess::*)(int))&QProcess::finished, [this, proc](int exitCode) {
        if (exitCode == 0) {
            emit this->mountedChanged();
        }
        delete proc;
    });

    return true;
}

bool CliDevice::isMounted() const
{
    if (type() != Type::Storage) {
        return false;
    }

    QByteArray out;
    run(QStringLiteral("findmnt %1").arg(udi()), &out);
    return !out.isEmpty();
}



CliBackend::CliBackend(HardwareManager *hw)
            : HardwareManager::Backend(hw)
{
}

CliBackend *CliBackend::create(HardwareManager *hw)
{
    CliBackend *cli = new CliBackend(hw);
    if (!cli) {
        return nullptr;
    }

    QByteArray out;
    run(QStringLiteral("lsblk -pPo NAME,FSTYPE | grep -iE 'FSTYPE=\".+\"' | grep  -Po '(?<=NAME=\").+?(?=\")'"), &out);
    QStringList list = QString::fromUtf8(out).split(QLatin1Char('\n'));
    list.removeLast();
    foreach (const QString &c, list) {
        bool isSwap = run(QStringLiteral("lsblk -o FSTYPE %1 | grep swap").arg(c)) == 0;
        Device *d = new CliDevice(c);
        if (!isSwap) {
            d->setType(Device::Type::Storage);
            bool isOptical = run(QStringLiteral("lsblk -o TYPE  %1 | grep rom").arg(c)) == 0;
            if (isOptical) {
                d->setIconName(QStringLiteral("media-optical"));
            } else {
                bool isRemovable = run(QStringLiteral("lsblk -o RM  %1 | grep 1").arg(c)) == 0;
                d->setIconName(isRemovable ? QStringLiteral("drive-removable-media") : QStringLiteral("drive-harddisk"));
            }

            QByteArray label;
            run(QStringLiteral("lsblk -no LABEL %1 | awk 1 ORS=''").arg(c), &label);
            label.isEmpty() ? d->setName(c) : d->setName(QString::fromUtf8(label));
        }
        cli->deviceAdded(d);
    }

    return cli;
}

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

#include <QDebug>
#include <QtQml>

#include <solid/devicenotifier.h>
#include <solid/storageaccess.h>

#include "hardwareservice.h"

Device::Device(const Solid::Device &dev)
      : QObject()
      , m_device(dev)
      , m_type(Type::None)
{
    if (Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>()) {
        m_type = Type::Storage;
        connect(access, &Solid::StorageAccess::setupDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            emit mountedChanged();
        });
        connect(access, &Solid::StorageAccess::teardownDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            emit mountedChanged();
        });
    }
}

bool Device::umount()
{
    if (m_type != Type::Storage) {
        return false;
    }

    Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->teardown();
}

bool Device::mount()
{
    if (m_type != Type::Storage) {
        return false;
    }

    Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->setup();
}

QString Device::udi() const
{
    return m_device.udi();
}

QString Device::name() const
{
    return m_device.description();
}

QString Device::iconName() const
{
    return m_device.icon();
}

bool Device::mounted() const
{
    if (m_type != Type::Storage) {
        return false;
    }

    const Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->isAccessible();
}



HardwareService::HardwareService()
               : Service()
{
    qmlRegisterUncreatableType<Device>("Orbital", 1, 0, "Device", "Cannot create Device");
}

HardwareService::~HardwareService()
{
    qDeleteAll(m_devices);
}

void HardwareService::init()
{
    Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    connect(notifier, &Solid::DeviceNotifier::deviceAdded, [this](const QString &udi) {
        Device *d = new Device(Solid::Device(udi));
        m_devices << d;
        emit devicesChanged();
        emit deviceAdded(d);
    });
    connect(notifier, &Solid::DeviceNotifier::deviceRemoved, [this](const QString &udi) {
        for (Device *d: m_devices) {
            if (d->udi() == udi) {
                m_devices.removeOne(d);
                d->deleteLater();
                emit devicesChanged();
                emit deviceRemoved(d);
                return;
            }
        }
    });

    for (Solid::Device &device: Solid::Device::allDevices()) {
        m_devices << new Device(device);
    }
}

QQmlListProperty<Device> HardwareService::devices()
{
    return QQmlListProperty<Device>(this, 0, devicesCount, devicesAt);
}

int HardwareService::devicesCount(QQmlListProperty<Device> *prop)
{
    HardwareService *h = static_cast<HardwareService *>(prop->object);
    return h->m_devices.count();
}

Device *HardwareService::devicesAt(QQmlListProperty<Device> *prop, int index)
{
    HardwareService *h = static_cast<HardwareService *>(prop->object);
    return h->m_devices.at(index);
}

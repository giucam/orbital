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

#include <solid/devicenotifier.h>
#include <solid/storageaccess.h>

#include "solidbackend.h"

SolidDevice::SolidDevice(const QString &udi)
           : Device(udi)
           , m_device(Solid::Device(udi))
{
    setName(m_device.description());
    setIconName(m_device.icon());

    if (Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>()) {
        setType(Type::Storage);
        connect(access, &Solid::StorageAccess::setupDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            emit mountedChanged();
        });
        connect(access, &Solid::StorageAccess::teardownDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
            emit mountedChanged();
        });
    }
}

bool SolidDevice::umount()
{
    if (type() != Type::Storage) {
        return false;
    }

    Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->teardown();
}

bool SolidDevice::mount()
{
    if (type() != Type::Storage) {
        return false;
    }

    Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->setup();
}

bool SolidDevice::isMounted() const
{
    if (type() != Type::Storage) {
        return false;
    }

    const Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    return access->isAccessible();
}



SolidBackend::SolidBackend(HardwareService *hw)
            : HardwareService::Backend(hw)
{
}

SolidBackend *SolidBackend::create(HardwareService *hw)
{
    SolidBackend *solid = new SolidBackend(hw);
    if (!solid) {
        return nullptr;
    }

    Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();
    QObject::connect(notifier, &Solid::DeviceNotifier::deviceAdded, [solid](const QString &udi) {
        Device *d = new SolidDevice(udi);
        solid->deviceAdded(d);
    });
    QObject::connect(notifier, &Solid::DeviceNotifier::deviceRemoved, [solid](const QString &udi) {
        solid->deviceRemoved(udi);
    });

    for (Solid::Device &device: Solid::Device::allDevices()) {
        Device *d = new SolidDevice(device.udi());
        solid->deviceAdded(d);
    }

    return solid;
}

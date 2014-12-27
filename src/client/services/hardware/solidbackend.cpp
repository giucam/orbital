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

#include <QDebug>

#include <solid/devicenotifier.h>
#include <solid/storageaccess.h>
#include <solid/battery.h>

#include "solidbackend.h"

SolidDevice::SolidDevice(const QString &udi)
           : Device(udi)
           , m_device(Solid::Device(udi))
{
    setName(m_device.description());
    setIconName(m_device.icon());
    setType(Type::Storage);
    Solid::StorageAccess *access = m_device.as<Solid::StorageAccess>();
    connect(access, &Solid::StorageAccess::setupDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
        emit mountedChanged();
    });
    connect(access, &Solid::StorageAccess::teardownDone, [this](Solid::ErrorType error, const QVariant &errorData, const QString &udi) {
        emit mountedChanged();
    });
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


static Battery::ChargeState fromSolid(Solid::Battery::ChargeState c)
{
    switch (c) {
        case Solid::Battery::FullyCharged:
        case Solid::Battery::NoCharge: return Battery::ChargeState::Stable;
        case Solid::Battery::Charging: return Battery::ChargeState::Charging;
        case Solid::Battery::Discharging: return Battery::ChargeState::Discharging;
    }
}

SolidBattery::SolidBattery(Solid::Battery *b, const QString &udi)
            : Battery(udi)
{
    Solid::Device d(udi);
    setName(d.description());
    setChargePercent(b->chargePercent());
    setChargeState(fromSolid(b->chargeState()));
    connect(b, &Solid::Battery::chargePercentChanged, [this](int charge, const QString &) {
        setChargePercent(charge);
    });
    connect(b, &Solid::Battery::chargeStateChanged, [this](int state, const QString &) {
        setChargeState(fromSolid((Solid::Battery::ChargeState)(state)));
    });
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
        Solid::Device d(udi);
        solid->add(d);
    });
    QObject::connect(notifier, &Solid::DeviceNotifier::deviceRemoved, [solid](const QString &udi) {
        solid->deviceRemoved(udi);
    });

    for (Solid::Device &device: Solid::Device::allDevices()) {
        solid->add(device);
    }

    return solid;
}

void SolidBackend::add(Solid::Device &device)
{
    if (device.as<Solid::StorageAccess>()) {
        Device *d = new SolidDevice(device.udi());
        deviceAdded(d);
    } else if (Solid::Battery *battery = device.as<Solid::Battery>()) {
        Battery *b = new SolidBattery(battery, device.udi());
        batteryAdded(b);
    }
}

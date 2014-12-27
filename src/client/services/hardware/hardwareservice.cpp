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
#include <QtQml>

#include "hardwareservice.h"
#include "clibackend.h"
#ifdef USE_SOLID
#include "solidbackend.h"
#endif

Device::Device(const QString &udi)
      : QObject()
      , m_type(Type::None)
      , m_udi(udi)
{
}

void Device::setType(Type t)
{
    m_type = t;
}

void Device::setName(const QString &name)
{
    m_name = name;
}

void Device::setIconName(const QString &name)
{
    m_icon = name;
}


Battery::Battery(const QString &udi)
       : QObject()
       , m_udi(udi)
       , m_chargePercent(0)
       , m_chargeState(ChargeState::Stable)
{
}

void Battery::setName(const QString &name)
{
    m_name = name;
}

void Battery::setChargePercent(int charge)
{
    if (m_chargePercent != charge) {
        m_chargePercent = charge;
        emit chargePercentChanged();
    }
}

void Battery::setChargeState(ChargeState cs)
{
    if (m_chargeState != cs) {
        m_chargeState = cs;
        emit chargeStateChanged();
    }
}



HardwareService::HardwareService()
               : Service()
               , m_backend(nullptr)
{
    qmlRegisterUncreatableType<Device>("Orbital", 1, 0, "Device", "Cannot create Device");
    qmlRegisterUncreatableType<Battery>("Orbital", 1, 0, "Battery", "Cannot create Battery");
}

HardwareService::~HardwareService()
{
    delete m_backend;
    qDeleteAll(m_devices);
}

void HardwareService::init()
{
#ifdef USE_SOLID
    m_backend = SolidBackend::create(this);
#endif
    if (!m_backend) {
        m_backend = CliBackend::create(this);
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
    return h->m_devices.values().at(index);
}

QQmlListProperty<Battery> HardwareService::batteries()
{
    auto batteriesCount = [](QQmlListProperty<Battery> *prop) {
        return static_cast<HardwareService *>(prop->object)->m_batteries.count();
    };
    auto batteryAt = [](QQmlListProperty<Battery> *prop, int i) {
        return static_cast<HardwareService *>(prop->object)->m_batteries.values().at(i);
    };
    return QQmlListProperty<Battery>(this, 0, batteriesCount, batteryAt);
}


HardwareService::Backend::Backend(HardwareService *hw)
                        : m_hw(hw)
{
}

void HardwareService::Backend::deviceAdded(Device *dev)
{
    m_hw->m_devices.insert(dev->udi(), dev);
    emit m_hw->devicesChanged();
    emit m_hw->deviceAdded(dev);
}

void HardwareService::Backend::batteryAdded(Battery *b)
{
    m_hw->m_batteries.insert(b->udi(), b);
    emit m_hw->batteryAdded(b);
    emit m_hw->batteriesChanged();
}

void HardwareService::Backend::deviceRemoved(const QString &udi)
{
    if (Device *d = m_hw->m_devices.take(udi)) {
        emit m_hw->devicesChanged();
        emit m_hw->deviceRemoved(d);
        d->deleteLater();
    } else if (Battery *b = m_hw->m_batteries.take(udi)) {
        emit m_hw->batteriesChanged();
        b->deleteLater();
    }
}

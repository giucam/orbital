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

#ifndef HARDWARESERVICE_H
#define HARDWARESERVICE_H

#include <QQmlListProperty>
#include <QQmlExtensionPlugin>

class HardwarePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri) override;
};


class Device : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString udi READ udi CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(Type type READ type CONSTANT)
    Q_PROPERTY(bool mounted READ isMounted NOTIFY mountedChanged)
public:
    enum class Type {
        None,
        Storage
    };
    Q_ENUMS(Type)

    Device(const QString &udi);
    virtual ~Device() {}

    Q_INVOKABLE virtual bool umount() = 0;
    Q_INVOKABLE virtual bool mount() = 0;

    QString udi() const { return m_udi; }
    QString name() const { return m_name; }
    QString iconName() const { return m_icon; }
    Type type() const { return m_type; }
    virtual bool isMounted() const = 0;

    void setType(Type t);
    void setName(const QString &name);
    void setIconName(const QString &name);

signals:
    void mountedChanged();

private:
    Type m_type;
    QString m_udi;
    QString m_name;
    QString m_icon;
};

class Battery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString udi READ udi CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int chargePercent READ chargePercent NOTIFY chargePercentChanged)
    Q_PROPERTY(ChargeState chargeState READ chargeState NOTIFY chargeStateChanged)
    Q_PROPERTY(qint64 timeToEmpty READ timeToEmpty NOTIFY timeToEmptyChanged)
public:
    enum class ChargeState {
        Stable,
        Charging,
        Discharging
    };
    Q_ENUMS(ChargeState)
    explicit Battery(const QString &udi);

    QString udi() const { return m_udi; }
    QString name() const { return m_name; }
    int chargePercent() const { return m_chargePercent; }
    ChargeState chargeState() const { return m_chargeState; }
    qint64 timeToEmpty() const { return m_timeToEmpty; }

protected:
    void setName(const QString &name);
    void setChargePercent(int charge);
    void setChargeState(ChargeState c);
    void setTimeToEmpty(qint64 time);

signals:
    void chargePercentChanged();
    void chargeStateChanged();
    void timeToEmptyChanged();

private:
    QString m_udi;
    QString m_name;
    int m_chargePercent;
    ChargeState m_chargeState;
    qint64 m_timeToEmpty;
};

class HardwareManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Device> devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QQmlListProperty<Battery> batteries READ batteries NOTIFY batteriesChanged)
public:
    class Backend
    {
    public:
        Backend(HardwareManager *hw);
        virtual ~Backend() {}

        void deviceAdded(Device *dev);
        void batteryAdded(Battery *b);
        void deviceRemoved(const QString &udi);

    private:
        HardwareManager *m_hw;
    };

    HardwareManager(QObject *p = nullptr);
    ~HardwareManager();

    QQmlListProperty<Device> devices();
    QQmlListProperty<Battery> batteries();

signals:
    void devicesChanged();
    void deviceAdded(Device *device);
    void deviceRemoved(Device *device);
    void batteriesChanged();
    void batteryAdded(Battery *battery);

private:
    Backend *m_backend;
    QMap<QString, Device *> m_devices;
    QMap<QString, Battery *> m_batteries;

    static int devicesCount(QQmlListProperty<Device> *prop);
    static Device *devicesAt(QQmlListProperty<Device> *prop, int index);
};

#endif

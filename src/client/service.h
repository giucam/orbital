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

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QHash>

class Service : public QObject
{
    Q_OBJECT
public:
    Service(QObject *p = nullptr);

};

class ServiceFactory
{
public:
    template<class T> static char registerService();

    static Service *createService(const QString &name);

private:
    typedef Service *(*Factory)();

    static void registerService(const QString &name, Factory factory);
    template<class T> static Service *factory() { return new T; }

    QHash<QString, Factory> m_factories;
};



#define REGISTER_SERVICE(class) static const int __r__ = ServiceFactory::registerService<class>();

template<class T>
char ServiceFactory::registerService() {
    static_assert(std::is_base_of<Service, T>::value, "Services must be a subclass of Service");

    registerService(QString(T::name()), factory<T>);
    return 0;
}

#endif

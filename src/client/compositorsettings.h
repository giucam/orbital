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

#ifndef COMPOSITORSETTING_H
#define COMPOSITORSETTING_H

#include <QObject>
#include <QHash>

class QXmlStreamReader;
class QXmlStreamWriter;

struct nuclear_settings;
struct nuclear_settings_listener;

class CompositorSettings : public QObject
{
    Q_OBJECT
public:
    explicit CompositorSettings(nuclear_settings *settings, QObject *p = nullptr);

    void load(QXmlStreamReader *xml);
    void save(QXmlStreamWriter *xml);

private:
    struct Option {
        enum class Type {
            String,
            Int,
            Binding
        };
        Option(Type t, int b = 0);
        Option() {}

        Type type;
        int bindingType;
        QString value;
        bool used;
    };

    bool set(const QString &name, const QString &v);
    void stringOption(nuclear_settings *settings, const char *path, const char *name);
    void intOption(nuclear_settings *settings,const char *path, const char *name);
    void bindingOption(nuclear_settings *settings,const char *path, const char *name, int types);

    nuclear_settings *m_settings;
    QHash<QString, Option> m_options;

    static const nuclear_settings_listener s_listener;
};

#endif

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

#include <linux/input.h>

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "compositorsettings.h"
#include "utils.h"

#include "wayland-settings-client-protocol.h"

CompositorSettings::CompositorSettings(nuclear_settings *settings, QObject *p)
                  : QObject(p)
                  , m_settings(settings)
{
    nuclear_settings_add_listener(m_settings, &s_listener, this);
}

void CompositorSettings::load(QXmlStreamReader *xml)
{
    while (!xml->atEnd()) {
        xml->readNext();
        if (xml->isStartElement()) {
            if (xml->name() == "option") {
                QXmlStreamAttributes attribs = xml->attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();
                set(name, value);
            }
        }
    }
}

void CompositorSettings::save(QXmlStreamWriter *xml)
{
    for (auto it = m_options.begin(); it != m_options.end(); ++it) {
        const Option &o = it.value();
        if (o.used) {
            xml->writeStartElement("option");
            xml->writeAttribute("name", it.key());
            xml->writeAttribute("value", o.value);
            xml->writeEndElement();
        }
    }
}

static bool typeFromString(const QString &t, nuclear_settings_binding_type *type)
{
    if (t == "key") {
        *type =  NUCLEAR_SETTINGS_BINDING_TYPE_KEY;
        return true;
    }
    if (t == "button") {
        *type = NUCLEAR_SETTINGS_BINDING_TYPE_BUTTON;
        return true;
    }
    if (t == "axis") {
        *type =  NUCLEAR_SETTINGS_BINDING_TYPE_AXIS;
        return true;
    }
    if (t == "hotspot") {
        *type =  NUCLEAR_SETTINGS_BINDING_TYPE_HOTSPOT;
        return true;
    }
    return false;
}

static bool modFromString(const QString &t, nuclear_settings_modifier *mod)
{
    if (t == "ctrl") {
        *mod = NUCLEAR_SETTINGS_MODIFIER_CTRL;
        return true;
    }
    if (t == "alt") {
        *mod = NUCLEAR_SETTINGS_MODIFIER_ALT;
        return true;
    }
    if (t == "super") {
        *mod = NUCLEAR_SETTINGS_MODIFIER_SUPER;
        return true;
    }
    if (t == "shift") {
        *mod = NUCLEAR_SETTINGS_MODIFIER_SHIFT;
        return true;
    }
    return false;
}

struct Key {
    const char *c;
    int k;
};

static Key keymap[] = {
    { "a", KEY_A },
    { "b", KEY_B },
    { "c", KEY_C },
    { "d", KEY_D },
    { "e", KEY_E },
    { "f", KEY_F },
    { "g", KEY_G },
    { "h", KEY_H },
    { "i", KEY_I },
    { "j", KEY_J },
    { "k", KEY_K },
    { "l", KEY_L },
    { "m", KEY_M },
    { "n", KEY_N },
    { "o", KEY_O },
    { "p", KEY_P },
    { "q", KEY_Q },
    { "r", KEY_R },
    { "s", KEY_S },
    { "t", KEY_T },
    { "v", KEY_V },
    { "u", KEY_U },
    { "w", KEY_W },
    { "x", KEY_X },
    { "y", KEY_Y },
    { "z", KEY_X },
    { "0", KEY_0 },
    { "1", KEY_1 },
    { "2", KEY_2 },
    { "3", KEY_3 },
    { "4", KEY_4 },
    { "5", KEY_5 },
    { "6", KEY_6 },
    { "7", KEY_7 },
    { "8", KEY_8 },
    { "9", KEY_9 },
    { "f1", KEY_F1 },
    { "f2", KEY_F2 },
    { "f3", KEY_F3 },
    { "f4", KEY_F4 },
    { "f5", KEY_F5 },
    { "f6", KEY_F6 },
    { "f7", KEY_F7 },
    { "f8", KEY_F8 },
    { "f9", KEY_F9 },
    { "f10", KEY_F10 },
    { "f11", KEY_F11 },
    { "f12", KEY_F12 },
    { "left", KEY_LEFT },
    { "right", KEY_RIGHT },
    { "up", KEY_UP },
    { "down", KEY_DOWN },
    { "pageup", KEY_PAGEUP },
    { "pagedown", KEY_PAGEDOWN },
    { "esc", KEY_ESC },
    { "minus", KEY_MINUS },
    { "space", KEY_SPACE },
    { "backspace", KEY_BACKSPACE },
    { "volumeup", KEY_VOLUMEUP },
    { "volumedown", KEY_VOLUMEDOWN }
};

static bool keyFromString(const QString &t, int *key)
{
    for (unsigned int i = 0; i < sizeof(keymap) / sizeof(Key); ++i) {
        if (keymap[i].c == t) {
            *key = keymap[i].k;
            return true;
        }
    }

    return false;
}

struct Button {
    const char *name;
    int b;
};

static const Button buttonmap[] = {
    { "button_left", BTN_LEFT },
    { "button_middle", BTN_MIDDLE },
    { "button_right", BTN_RIGHT }
};

static bool buttonFromString(const QString &t, int *button)
{
    for (unsigned int i = 0; i < sizeof(buttonmap) / sizeof(Button); ++i) {
        if (t == buttonmap[i].name) {
            *button = buttonmap[i].b;
            return true;
        }
    }

    return false;
}

static bool hsFromString(const QString &t, nuclear_settings_hotspot *hs)
{
    if (t == "topleft_corner") {
        *hs = NUCLEAR_SETTINGS_HOTSPOT_TOP_LEFT_CORNER;
        return true;
    }
    if (t == "topright_corner") {
        *hs = NUCLEAR_SETTINGS_HOTSPOT_TOP_RIGHT_CORNER;
        return true;
    }
    if (t == "bottomleft_corner") {
        *hs = NUCLEAR_SETTINGS_HOTSPOT_BOTTOM_LEFT_CORNER;
        return true;
    }
    if (t == "bottomright_corner") {
        *hs = NUCLEAR_SETTINGS_HOTSPOT_BOTTOM_RIGHT_CORNER;
        return true;
    }
    return false;
}

static bool axisFromString(const QString &t, int *axis)
{
    if (t == "axis_vertical") {
        *axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
        return true;
    }
    if (t == "axis_horizontal") {
        *axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
        return true;
    }
    return false;
}

bool CompositorSettings::set(const QString &option, const QString &v)
{
    if (!m_options.contains(option)) {
        return false;
    }

    QStringList parts = option.split('.');
    QString path = parts.at(0);
    QString name = parts.at(1);

    Option &o = m_options[option];
    if (o.type == Option::Type::String) {
        nuclear_settings_set_string(m_settings, qPrintable(path), qPrintable(name), qPrintable(v));
    } else if (o.type == Option::Type::Int) {
        nuclear_settings_set_integer(m_settings, qPrintable(path), qPrintable(name), v.toInt());
    } else {
        QStringList binds = v.toLower().split(';');
        for (const QString &b: binds) {
            QStringList s = b.split(':');
            nuclear_settings_binding_type type;
            if (!typeFromString(s.at(0), &type)) {
                return false;
            }

            QString binding = s.at(1);
            if (type == NUCLEAR_SETTINGS_BINDING_TYPE_KEY) {
                nuclear_settings_modifier mod = (nuclear_settings_modifier)0;
                int key;
                for (const QString &part: binding.split('+')) {
                    nuclear_settings_modifier m;
                    if (modFromString(part, &m)) {
                        mod = (nuclear_settings_modifier)(mod | m);
                    } else if (!keyFromString(part, &key)) {
                        return false;
                    }
                }
                nuclear_settings_set_key_binding(m_settings, qPrintable(path), qPrintable(name), key, mod);
            } else if (type == NUCLEAR_SETTINGS_BINDING_TYPE_BUTTON) {
                nuclear_settings_modifier mod = (nuclear_settings_modifier)0;
                int button;
                for (const QString &part: binding.split('+')) {
                    nuclear_settings_modifier m;
                    if (modFromString(part, &m)) {
                        mod = (nuclear_settings_modifier)(mod | m);
                    } else if (!buttonFromString(part, &button)) {
                        return false;
                    }
                }
                nuclear_settings_set_button_binding(m_settings, qPrintable(path), qPrintable(name), button, mod);
            } else if (type == NUCLEAR_SETTINGS_BINDING_TYPE_AXIS) {
                nuclear_settings_modifier mod = (nuclear_settings_modifier)0;
                int axis;
                for (const QString &part: binding.split('+')) {
                    nuclear_settings_modifier m;
                    if (modFromString(part, &m)) {
                        mod = (nuclear_settings_modifier)(mod | m);
                    } else if (!axisFromString(part, &axis)) {
                        return false;
                    }
                }
                nuclear_settings_set_axis_binding(m_settings, qPrintable(path), qPrintable(name), axis, mod);
            } else if (type == NUCLEAR_SETTINGS_BINDING_TYPE_HOTSPOT) {
                nuclear_settings_hotspot hs;
                if (!hsFromString(binding, &hs)) {
                    return false;
                }
                nuclear_settings_set_hotspot_binding(m_settings, qPrintable(path), qPrintable(name), hs);
            }
        }
    }
    o.used = true;
    o.value = v;
    return true;
}

void CompositorSettings::stringOption(nuclear_settings *settings, const char *path, const char *name)
{
    m_options.insert(QString(path) + "." + name, Option(Option::Type::String));
}

void CompositorSettings::intOption(nuclear_settings *settings, const char *path, const char *name)
{
    m_options.insert(QString(path) + "." + name, Option(Option::Type::Int));
}

void CompositorSettings::bindingOption(nuclear_settings *settings, const char *path, const char *name, int types)
{
    m_options.insert(QString(path) + "." + name, Option(Option::Type::Binding, types));
}

const nuclear_settings_listener CompositorSettings::s_listener = {
    wrapInterface(&CompositorSettings::stringOption),
    wrapInterface(&CompositorSettings::intOption),
    wrapInterface(&CompositorSettings::bindingOption)
};

CompositorSettings::Option::Option(Type t, int b)
                          : type(t)
                          , bindingType(b)
                          , used(false)
{
}

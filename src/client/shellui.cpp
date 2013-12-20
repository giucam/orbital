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

#include "shellui.h"

#include <QIcon>
#include <QDebug>
#include <QQuickItem>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QCoreApplication>
#include <QQuickWindow>
#include <QGuiApplication>

#include "client.h"
#include "element.h"
#include "uiscreen.h"
#include "style.h"
#include "compositorsettings.h"

static const char *defaultConfig =
"<Orbital>\n"
"    <Ui>\n"
"        <property name=\"iconTheme\" value=\"oxygen\"/>\n"
"        <property name=\"numWorkspaces\" value=\"4\"/>\n"
"        <property name=\"styleName\" value=\"chiaro\"/>\n"
"        <Screen>\n"
"            <element type=\"background\" id=\"1\">\n"
"                <property name=\"color\" value=\"black\"/>\n"
"                <property name=\"imageSource\" value=\"/usr/share/weston/pattern.png\"/>\n"
"                <property name=\"imageFillMode\" value=\"4\"/>\n"
"            </element>\n"
"            <element type=\"panel\" id=\"2\">\n"
"                <property name=\"location\" value=\"0\"/>\n"
"                <element type=\"launcher\" id=\"3\">\n"
"                    <property name=\"icon\" value=\"image://icon/utilities-terminal\"/>\n"
"                    <property name=\"process\" value=\"/usr/bin/weston-terminal\"/>\n"
"                </element>\n"
"                <element type=\"pager\" id=\"4\"/>\n"
"                <element type=\"taskbar\" id=\"5\"/>\n"
"                <element type=\"mixer\" id=\"6\"/>\n"
"                <element type=\"logout\" id=\"7\"/>\n"
"                <element type=\"clock\" id=\"8\"/>\n"
"            </element>\n"
"            <element type=\"overlay\" id=\"9\"/>\n"
"        </Screen>\n"
"    </Ui>\n"
"    <CompositorSettings>\n"
"        <option name=\"effects/scale_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"effects/scale_effect.toggle_binding\" value=\"key:ctrl+e;hotspot:topleft_corner\"/>\n"
"        <option name=\"effects/griddesktops_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"effects/griddesktops_effect.toggle_binding\" value=\"key:ctrl+g;hotspot:topright_corner\"/>\n"
"        <option name=\"effects/zoom_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"effects/zoom_effect.zoom_binding\" value=\"axis:super+axis_vertical\"/>\n"
"        <option name=\"effects/minimize_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"effects/inoutsurface_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"effects/fademoving_effect.enabled\" value=\"1\"/>\n"
"        <option name=\"desktop_shell.move_window\" value=\"button:super+button_left\"/>\n"
"        <option name=\"desktop_shell.resize_window\" value=\"button:super+button_middle\"/>\n"
"        <option name=\"desktop_shell.previous_workspace\" value=\"key:ctrl+left\"/>\n"
"        <option name=\"desktop_shell.next_workspace\" value=\"key:ctrl+right\"/>\n"
"    </CompositorSettings>\n"
"</Orbital>\n";

ShellUI::ShellUI(Client *client, CompositorSettings *s, QQmlEngine *engine, const QString &configFile)
       : QObject(client)
       , m_client(client)
       , m_compositorSettings(s)
       , m_configFile(configFile)
       , m_configMode(false)
       , m_cursorShape(-1)
       , m_engine(engine)
       , m_numWorkspaces(1)
       , m_style(nullptr)
{
    client->addWorkspace(0);
    reloadConfig();
}

ShellUI::~ShellUI()
{
    qDeleteAll(m_screens);
}

UiScreen *ShellUI::loadScreen(int s, QScreen *sc)
{
    m_engine->rootContext()->setContextProperty("Ui", this);

    qDebug()<<"screen"<<s;

    UiScreen *screen = new UiScreen(this, m_client, s, sc);
    loadScreen(screen);

    m_screens << screen;
    return screen;
}

UiScreen *ShellUI::findScreen(wl_output *output) const
{
    for (UiScreen *screen: m_screens) {
        QScreen *sc = screen->screen();
        wl_output *out = Client::nativeOutput(sc);
        if (out == output) {
            return screen;
        }
    }

    return nullptr;
}

QString ShellUI::iconTheme() const
{
    return QIcon::themeName();
}

void ShellUI::setIconTheme(const QString &theme)
{
    QIcon::setThemeName(theme);
}

void ShellUI::setNumWorkspaces(int n)
{
    if (n > 0) {
        for (;m_numWorkspaces < n; ++m_numWorkspaces) {
            m_client->addWorkspace(m_numWorkspaces);

        }
        while (m_numWorkspaces > n) {
            --m_numWorkspaces;
            m_client->removeWorkspace(m_numWorkspaces);
        }
    }
}

Element *ShellUI::createElement(const QString &name)
{
    Element *elm = Element::create(this, m_engine, name);
    return elm;
}

void ShellUI::setConfigMode(bool mode)
{
    m_configMode = mode;
    emit configModeChanged();
    if (!m_configMode) {
        saveConfig();
    }
}

void ShellUI::setStyleName(const QString &name)
{
    if (name == m_styleName) {
        return;
    }

    delete m_style;
    m_style = Style::loadStyle(name, m_engine);
    m_styleName = name;
    m_engine->rootContext()->setContextProperty("CurrentStyle", m_style);
}

void ShellUI::setOverrideCursorShape(Qt::CursorShape shape)
{
    if (m_cursorShape == (int)shape) {
        return;
    }

    QGuiApplication::restoreOverrideCursor();
    QGuiApplication::setOverrideCursor(QCursor(shape));
    m_cursorShape = (int)shape;
}

void ShellUI::restoreOverrideCursorShape()
{
    QGuiApplication::restoreOverrideCursor();
    m_cursorShape = -1;
}

void ShellUI::toggleConfigMode()
{
    setConfigMode(!m_configMode);
}

static void goToEndElement(QXmlStreamReader &xml)
{
    QString name = xml.name().toString();
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && name == xml.name()) {
            goToEndElement(xml);
        } else if (name == xml.name()) {
            break;
        }
    }
}

void ShellUI::reloadConfig()
{
    m_properties.clear();

    QFile file(m_configFile);
    if (file.open(QIODevice::ReadOnly)) {
        m_configData = file.readAll();
        file.close();
    } else {
        m_configData = defaultConfig;
    }

    QXmlStreamReader xml(m_configData);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "property") {
                QXmlStreamAttributes attribs = xml.attributes();
                QString name = attribs.value("name").toString();
                QString value = attribs.value("value").toString();

                setProperty(qPrintable(name), value);
                m_properties << name;
            } else if (xml.name() == "CompositorSettings") {
                if (m_compositorSettings) {
                    m_compositorSettings->load(&xml);
                }
            } else if (xml.name() != "Ui" && xml.name() != "Orbital") {
                goToEndElement(xml);
            }
        }
    }

    for (UiScreen *screen: m_screens) {
        loadScreen(screen);
    }
}

void ShellUI::saveConfig()
{
    QFile file(m_configFile);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xml(&file);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("Orbital");
    xml.writeStartElement("Ui");

    for (const QString &prop: m_properties) {
        xml.writeStartElement("property");
        xml.writeAttribute("name", prop);
        xml.writeAttribute("value", property(qPrintable(prop)).toString());
        xml.writeEndElement();
    }

    for (UiScreen *screen: m_screens) {
        screen->saveConfig(xml);
    }

    xml.writeEndElement();
    xml.writeStartElement("CompositorSettings");
    if (m_compositorSettings) {
        m_compositorSettings->save(&xml);
    }
    xml.writeEndElement();
    xml.writeEndElement();
    file.close();
}

void ShellUI::loadScreen(UiScreen *screen)
{
    QXmlStreamReader xml(m_configData);

    bool loaded = false;
    while (!xml.atEnd()) {
        if (!xml.readNextStartElement()) {
            continue;
        }
        if (xml.name() == "Screen") {
            QXmlStreamAttributes attribs = xml.attributes();
            if (attribs.hasAttribute("output")) {
                int num = attribs.value("output").toInt();
                if (num == screen->id()) {
                    screen->loadConfig(xml);
                    loaded = true;
                    break;
                }
            }
        }
    }

    if (!loaded) {
        QXmlStreamReader xml(defaultConfig);
        while (!xml.atEnd()) {
            if (!xml.readNextStartElement()) {
                continue;
            }
            if (xml.name() == "Screen") {
                screen->loadConfig(xml);
                return;
            }
        }
    }
}

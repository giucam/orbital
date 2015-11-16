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

#include "shellui.h"

#include <QIcon>
#include <QDebug>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QCoreApplication>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QScreen>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include "client.h"
#include "element.h"
#include "uiscreen.h"
#include "style.h"
#include "compositorsettings.h"

const char *defaultShell =
"{\n"
"    \"properties\": {\n"
"        \"numWorkspaces\": 4,\n"
"        \"styleName\": \"chiaro\"\n"
"    }\n"
"}\n";

const char *defaultScreen =
"{\n"
"    \"elements\": [\n"
"        {\n"
"            \"type\": \"background\",\n"
"            \"properties\": {\n"
"                \"color\": \"#000000\",\n"
"                \"imageSource\": \"/usr/local/share/weston/pattern.png\",\n"
"                \"imageFillMode\": 4\n"
"            },\n"
"            \"elements\" : []\n"
"        },\n"
"        {\n"
"            \"type\": \"panel\",\n"
"            \"properties\": { \"location\": 0 },\n"
"            \"elements\": [\n"
"                {\n"
"                    \"type\": \"launcher\",\n"
"                    \"properties\": {\n"
"                        \"icon\": \"image://icon/utilities-terminal\",\n"
"                        \"process\": \"weston-terminal\"\n"
"                    }\n"
"                },\n"
"                { \"type\": \"pager\" },\n"
"                { \"type\": \"taskbar\" },\n"
"                { \"type\": \"mixer\" },\n"
"                { \"type\": \"logout\" },\n"
"                { \"type\": \"clock\" }\n"
"            ]\n"
"        },\n"
"        { \"type\": \"overlay\" },\n"
"        { \"type\" : \"lockscreen\" } \n"
"    ]\n"
"}\n";

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
    m_engine->rootContext()->setContextProperty(QStringLiteral("Ui"), this);

    client->addWorkspace(0);
    reloadConfigFile();
    reloadConfig();
}

ShellUI::~ShellUI()
{
    qDeleteAll(m_screens);
}

UiScreen *ShellUI::loadScreen(QScreen *sc, const QString &name)
{
    qDebug()<<"screen"<<name;

    UiScreen *screen = new UiScreen(this, m_client, sc, name);
    loadScreen(screen);

    m_screens << screen;
    connect(sc, &QObject::destroyed, [this, screen](QObject *) { delete screen; m_screens.removeOne(screen); });
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

Element *ShellUI::createElement(const QString &name, UiScreen *screen)
{
    Element *elm = Element::create(this, screen, m_engine, name);
    return elm;
}

void ShellUI::setConfigMode(bool mode)
{
    if (m_configMode == mode) {
        return;
    }

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
    m_engine->rootContext()->setContextProperty(QStringLiteral("CurrentStyle"), m_style);
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

void ShellUI::reloadConfig()
{
    m_properties.clear();
    qDeleteAll(m_bindings);
    m_bindings.clear();

    QJsonObject object;
    if (m_config.contains(QStringLiteral("Shell"))) {
        object = m_config[QStringLiteral("Shell")].toObject();
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(defaultShell);
        object = doc.object();
    }
    QJsonObject properties = object[QStringLiteral("properties")].toObject();
    for (auto i = properties.constBegin(); i != properties.constEnd(); ++i) {
        setProperty(qPrintable(i.key()), i.value().toVariant());
        m_properties << i.key();
    }

    QJsonArray bindings = object[QStringLiteral("bindings")].toArray();
    for (auto i = bindings.begin(); i != bindings.end(); ++i) {
        QJsonObject binding = (*i).toObject();
        int key = binding[QStringLiteral("key")].toInt(-1);
        QString exec = binding[QStringLiteral("exec")].toString();
        if (key < 0 || exec.isEmpty()) {
            qDebug() << "Cannot parse binding" << binding;
            continue;
        }
        Binding *b = m_client->addKeyBinding(key, 0);
        m_bindings << b;
        connect(b, &Binding::triggered, [exec]() { QProcess::startDetached(exec); });
    }

    foreach (UiScreen *screen, m_screens) {
        loadScreen(screen);
    }
}

void ShellUI::reloadConfigFile()
{
    m_rootConfig = QJsonObject();
    m_config = QJsonObject();

    QFile file(m_configFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning("Error parsing the config file at offset %d: %s", error.offset, qPrintable(error.errorString()));
        } else {
            m_rootConfig = document.object();
            m_config = m_rootConfig[QStringLiteral("Ui")].toObject();
        }
        file.close();
    }
}

void ShellUI::saveConfig()
{
    QJsonObject object = m_config[QStringLiteral("Shell")].toObject();
    QJsonObject properties = object[QStringLiteral("properties")].toObject();
    foreach (const QString &prop, m_properties) {
        QVariant value = property(qPrintable(prop));
        bool ok;
        int v = value.toInt(&ok);
        if (ok) {
            properties[prop] = v;
        } else {
            properties[prop] = value.toString();
        }
    }
    object[QStringLiteral("properties")] = properties;

    QJsonObject screens = m_config[QStringLiteral("Screens")].toObject();
    foreach (UiScreen *screen, m_screens) {
        QJsonObject screenConfig = screens[screen->name()].toObject();
        screen->saveConfig(screenConfig);
        screens[screen->name()] = screenConfig;
    }
    m_config[QStringLiteral("Screens")] = screens;

    m_config[QStringLiteral("Shell")] = object;
    m_rootConfig[QStringLiteral("Ui")] = m_config;

    QJsonDocument document(m_rootConfig);

    QFile file(m_configFile);
    QFileInfo info(file);
    QDir dir(info.absoluteDir());
    if (!dir.exists() && !dir.mkpath(dir.path())) {
        qWarning("Failed to create the config directory '%s', cannot save the config.", qPrintable(dir.path()));
        return;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Failed to open %s for writing.", qPrintable(m_configFile));
        return;
    }

    QByteArray data = document.toJson();
    int pos = 0;
    while (pos < data.size()) {
        int index = data.indexOf('\n', pos);
        QByteArray line = data.mid(pos, index - pos + 1);
        pos = index + 1;

        //filter out the ids from the saved config
        if (line.contains("\"id\":")) {
            continue;
        }
        file.write(line);
    }

    qDebug("Saved Orbital config to %s.", qPrintable(m_configFile));
}

void ShellUI::loadScreen(UiScreen *screen)
{
    QJsonObject screens = m_config[QStringLiteral("Screens")].toObject();
    QJsonObject screenConfig;
    if (screens.contains(screen->name())) {
        screenConfig = screens[screen->name()].toObject();
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(defaultScreen);
        screenConfig = doc.object();
    }

    screen->loadConfig(screenConfig);
    screens[screen->name()] = screenConfig;

    m_config[QStringLiteral("Screens")] = screens;
}

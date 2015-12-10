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
#include <QtQml>
#include <qmath.h>

#include "mixerservice.h"
#include "client.h"

#ifdef HAVE_ALSA
#include "alsamixer.h"
#endif
#include "pulseaudiomixer.h"

void MixerPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<Mixer>(uri, 1, 0, "Mixer", [](QQmlEngine *, QJSEngine *) {
        return static_cast<QObject *>(new Mixer);
    });
}



Mixer::Mixer(QObject *p)
            : QObject(p)
            , m_backend(nullptr)
{
    m_backend = PulseAudioMixer::create(this);
    if (!m_backend) {
#ifdef HAVE_ALSA
        m_backend = AlsaMixer::create(this);
#endif
    }
    if (!m_backend) {
        qWarning() << "Mixer: could not load a mixer backend.";
        return;
    }

    m_backend->getBoundaries(&m_min, &m_max);
    m_step = (m_max - m_min) / 50;

    Client::client()->addAction("Mixer.increaseVolume", [this]() { increaseMaster(); emit bindingTriggered(); });
    Client::client()->addAction("Mixer.decreaseVolume", [this]() { decreaseMaster(); emit bindingTriggered(); });
    Client::client()->addAction("Mixer.toggleMuted", [this]() { toggleMuted(); emit bindingTriggered(); });
}

Mixer::~Mixer()
{
    delete m_backend;
}

void Mixer::changeMaster(int change)
{
    setMaster(master() + change);
}

void Mixer::increaseMaster()
{
    if (m_backend) {
        long v = qBound(m_min, m_backend->rawVol() + m_step, m_max);
        m_backend->setRawVol(v);
    }
}

void Mixer::decreaseMaster()
{
    if (m_backend) {
        long v = qBound(m_min, m_backend->rawVol() - m_step, m_max);
        m_backend->setRawVol(v);
    }
}

void Mixer::setMaster(int volume)
{
    if (m_backend) {
        int v = qBound(m_min, (int)((double)volume * (double)m_max / 100.), m_max);
        m_backend->setRawVol(v);
    }
}

int Mixer::master() const
{
    if (m_backend) {
        int vol = m_backend->rawVol();
        vol = (float)vol * 100.f / (float)m_max;
        return vol;
    }
    return 0;
}

bool Mixer::muted() const
{
    if (m_backend) {
        return m_backend->muted();
    }
    return false;
}

void Mixer::setMuted(bool muted)
{
    if (m_backend) {
        m_backend->setMuted(muted);
    }
}

void Mixer::toggleMuted()
{
    if (m_backend) {
        setMuted(!m_backend->muted());
    }
}

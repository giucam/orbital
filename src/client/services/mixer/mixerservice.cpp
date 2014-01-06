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

#include "mixerservice.h"
#include "client.h"

#ifdef HAVE_ALSA
#include "alsamixer.h"
#endif

MixerService::MixerService()
            : Service()
            , m_backend(nullptr)
{

}

MixerService::~MixerService()
{
    delete m_backend;
    delete m_upBinding;
    delete m_downBinding;
    delete m_muteBinding;
}

void MixerService::init()
{
#ifdef HAVE_ALSA
    m_backend = AlsaMixer::create(this);
#endif
    if (!m_backend) {
        qWarning() << "MixerService: could not load a mixer backend.";
        return;
    }

    m_backend->getBoundaries(&m_min, &m_max);

    m_upBinding = client()->addKeyBinding(KEY_VOLUMEUP, 0);
    m_downBinding = client()->addKeyBinding(KEY_VOLUMEDOWN, 0);
    m_muteBinding = client()->addKeyBinding(KEY_MUTE, 0);

    connect(m_upBinding, &Binding::triggered, [this]() { increaseMaster(); emit bindingTriggered(); });
    connect(m_downBinding, &Binding::triggered, [this]() { decreaseMaster(); emit bindingTriggered(); });
    connect(m_muteBinding, &Binding::triggered, [this]() { toggleMuted(); emit bindingTriggered(); });
}

void MixerService::changeMaster(int change)
{
    setMaster(master() + change);
}

void MixerService::increaseMaster()
{
    if (m_backend) {
        m_backend->setRawVol(m_backend->rawVol() + 2);
        emit masterChanged();
    }
}

void MixerService::decreaseMaster()
{
    if (m_backend) {
        m_backend->setRawVol(m_backend->rawVol() - 2);
        emit masterChanged();
    }
}

void MixerService::setMaster(int volume)
{
    if (m_backend) {
        m_backend->setRawVol((float)volume * (float)m_max / 100.f);
        emit masterChanged();
    }
}

int MixerService::master() const
{
    if (m_backend) {
        int vol = m_backend->rawVol();
        vol = (float)vol * 100.f / (float)m_max;
        return vol;
    }
    return 0;
}

bool MixerService::muted() const
{
    if (m_backend) {
        return m_backend->muted();
    }
    return false;
}

void MixerService::setMuted(bool muted)
{
    if (m_backend) {
        m_backend->setMuted(muted);
        emit mutedChanged();
    }
}

void MixerService::toggleMuted()
{
    if (m_backend) {
        setMuted(!m_backend->muted());
    }
}

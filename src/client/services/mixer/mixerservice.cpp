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

const char *card = "default";
const char *selem_name = "Master";

MixerService::MixerService()
            : Service()
{

}

MixerService::~MixerService()
{
    snd_mixer_close(m_handle);

    delete m_upBinding;
    delete m_downBinding;
    delete m_muteBinding;
}

void MixerService::init()
{
    snd_mixer_open(&m_handle, 0);
    snd_mixer_attach(m_handle, card);
    snd_mixer_selem_register(m_handle, NULL, NULL);
    snd_mixer_load(m_handle);

    snd_mixer_selem_id_alloca(&m_sid);
    snd_mixer_selem_id_set_index(m_sid, 0);
    snd_mixer_selem_id_set_name(m_sid, selem_name);
    m_elem = snd_mixer_find_selem(m_handle, m_sid);
    snd_mixer_selem_get_playback_volume_range(m_elem, &m_min, &m_max);

    m_upBinding = client()->addKeyBinding(KEY_VOLUMEUP, 0);
    m_downBinding = client()->addKeyBinding(KEY_VOLUMEDOWN, 0);
    m_muteBinding = client()->addKeyBinding(KEY_MUTE, 0);

    connect(m_upBinding, &Binding::triggered, [this]() { increaseMaster(); });
    connect(m_downBinding, &Binding::triggered, [this]() { decreaseMaster(); });
    connect(m_muteBinding, &Binding::triggered, this, &MixerService::toggleMuted);
}

void MixerService::changeMaster(int change)
{
    setMaster(master() + change);
}

void MixerService::increaseMaster()
{
    setRawVol(rawVol() + 2);
}

void MixerService::decreaseMaster()
{
    setRawVol(rawVol() - 2);
}

void MixerService::setRawVol(int volume)
{
    if (volume > m_max) volume = m_max;
    if (volume < m_min) volume = m_min;
    snd_mixer_selem_set_playback_volume_all(m_elem, volume);

    emit masterChanged();
}

void MixerService::setMaster(int volume)
{
    setRawVol((float)volume * (float)m_max / 100.f);

    emit masterChanged();
}

int MixerService::rawVol() const
{
    long vol;
    snd_mixer_selem_get_playback_volume(m_elem, SND_MIXER_SCHN_UNKNOWN, &vol);
    return vol;
}

int MixerService::master() const
{
    int vol = rawVol();
    vol = (float)vol * 100.f / (float)m_max;
    return vol;
}

bool MixerService::muted() const
{
    int mute;
    snd_mixer_selem_get_playback_switch(m_elem, SND_MIXER_SCHN_UNKNOWN, &mute);
    return !mute;
}

void MixerService::setMuted(bool muted)
{
    snd_mixer_selem_set_playback_switch_all(m_elem, !muted);
    emit mutedChanged();
}

void MixerService::toggleMuted()
{
    setMuted(!muted());
}

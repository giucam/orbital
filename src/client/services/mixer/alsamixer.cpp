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

#include "alsamixer.h"
#include "client.h"

const char *card = "default";
const char *selem_name = "Master";

AlsaMixer::AlsaMixer(MixerService *m)
         : Backend()
         , m_mixer(m)
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
}

AlsaMixer::~AlsaMixer()
{
    snd_mixer_close(m_handle);
}

void AlsaMixer::getBoundaries(long *min, long *max) const
{
    *min = m_min;
    *max = m_max;
}

void AlsaMixer::setRawVol(int volume)
{
    if (volume > m_max) volume = m_max;
    if (volume < m_min) volume = m_min;
    snd_mixer_selem_set_playback_volume_all(m_elem, volume);
}

int AlsaMixer::rawVol() const
{
    long vol;
    snd_mixer_selem_get_playback_volume(m_elem, SND_MIXER_SCHN_UNKNOWN, &vol);
    return vol;
}

bool AlsaMixer::muted() const
{
    int mute;
    snd_mixer_selem_get_playback_switch(m_elem, SND_MIXER_SCHN_UNKNOWN, &mute);
    return !mute;
}

void AlsaMixer::setMuted(bool muted)
{
    snd_mixer_selem_set_playback_switch_all(m_elem, !muted);
}

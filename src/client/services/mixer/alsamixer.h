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

#ifndef ALSAMIXER_H
#define ALSAMIXER_H

#include <alsa/asoundlib.h>

#include "mixerservice.h"

class AlsaMixer : public Backend
{
public:
    AlsaMixer(MixerService *mixer);
    ~AlsaMixer();

    void getBoundaries(long *min, long *max) const override;

    int rawVol() const override;
    void setRawVol(int vol) override;
    bool muted() const override;
    void setMuted(bool muted) override;

private:
    MixerService *m_mixer;
    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *m_sid;
    snd_mixer_elem_t *m_elem;
    long m_min;
    long m_max;
};

#endif

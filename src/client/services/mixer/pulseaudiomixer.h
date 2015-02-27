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

#ifndef PULSEAUDIOMIXER_H
#define PULSEAUDIOMIXER_H

#include <pulse/pulseaudio.h>

#include "mixerservice.h"

struct pa_glib_mainloop;

struct Sink;

class PulseAudioMixer : public Backend
{
public:
    ~PulseAudioMixer();

    static PulseAudioMixer *create(Mixer *mixer);

    void getBoundaries(int *min, int *max) const override;

    int rawVol() const override;
    void setRawVol(int vol) override;
    bool muted() const override;
    void setMuted(bool muted) override;

private:
    PulseAudioMixer(Mixer *mixer);
    void contextStateCallback(pa_context *c);
    void subscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t index);
    void sinkCallback(pa_context *c, const pa_sink_info *i, int eol);
    void cleanup();

    Mixer *m_mixer;
    pa_glib_mainloop *m_mainLoop;
    pa_mainloop_api *m_mainloopApi;
    pa_context *m_context;
    Sink *m_sink;
};

#endif

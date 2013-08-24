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

#ifndef VOLUMECONTROL_H
#define VOLUMECONTROL_H

#include <alsa/asoundlib.h>

#include "service.h"

class Binding;

class MixerService : public Service
{
    Q_OBJECT
    Q_PROPERTY(int master READ master WRITE setMaster NOTIFY masterChanged);
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_INTERFACES(Service)
    Q_PLUGIN_METADATA(IID "Orbital.Service")
public:
    MixerService();
    ~MixerService();

    void init();

    int master() const;
    bool muted() const;
    void setMuted(bool muted);

    constexpr static const char *name() { return "MixerService"; }

public slots:
    void increaseMaster();
    void decreaseMaster();
    void setMaster(int master);
    void changeMaster(int change);
    void toggleMuted();

signals:
    void masterChanged();
    void mutedChanged();

private:
    int rawVol() const;
    void setRawVol(int vol);

    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *m_sid;
    snd_mixer_elem_t *m_elem;
    long m_min;
    long m_max;
    int m_savedVol;

    Binding *m_upBinding;
    Binding *m_downBinding;
    Binding *m_muteBinding;
};

#endif

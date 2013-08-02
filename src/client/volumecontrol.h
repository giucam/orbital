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
 * Nome-Programma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VOLUMECONTROL_H
#define VOLUMECONTROL_H

#include <alsa/asoundlib.h>

#include <QObject>

class VolumeControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int master READ master WRITE setMaster NOTIFY masterChanged);
public:
    VolumeControl(QObject *parent = nullptr);
    ~VolumeControl();

    int master() const;

public slots:
    void setMaster(int master);
    void changeMaster(int change);

signals:
    void masterChanged();

private:
    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *m_sid;
    snd_mixer_elem_t *m_elem;
    long m_min;
    long m_max;
};

#endif

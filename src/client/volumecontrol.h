
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

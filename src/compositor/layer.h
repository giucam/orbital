
#ifndef ORBITAL_LAYER_H
#define ORBITAL_LAYER_H

#include <QObject>

struct weston_layer;

namespace Orbital {

class View;

class Layer : public QObject
{
    Q_OBJECT
public:
    explicit Layer(weston_layer *layer);
    explicit Layer(Layer *p = nullptr);

    void append(Layer *l);

    void addView(View *view);

    void setMask(int x, int y, int w, int h);

private:
    weston_layer *m_layer;

};

}

#endif


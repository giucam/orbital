
#ifndef ORBITAL_OUTPUT_H
#define ORBITAL_OUTPUT_H

#include <QObject>

struct weston_output;

namespace Orbital {

class Workspace;
class View;

class Output : public QObject
{
    Q_OBJECT
public:
    explicit Output(weston_output *out);

    void viewWorkspace(Workspace *ws);

    int id() const;
    int width() const;
    int height() const;

private:
    weston_output *m_output;

    friend View;
};

}

#endif

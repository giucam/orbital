
#ifndef SHELLITEM_H
#define SHELLITEM_H

#include <QQuickWindow>

class ShellItem : public QQuickWindow
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type WRITE setType)
public:
    enum Type {
        None,
        Background,
        Panel,
        Overlay
    };
    Q_ENUMS(Type)
    ShellItem(QWindow *parent = nullptr);

    inline Type type() const { return m_type; }
    inline void setType(Type t) { m_type = t; }

private:
    Type m_type;
};

#endif

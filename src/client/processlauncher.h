
#ifndef PROCESSLAUNCHER_H
#define PROCESSLAUNCHER_H

#include <QObject>

class ProcessLauncher : public QObject
{
    Q_OBJECT
public:
    ProcessLauncher(QObject *parent = nullptr);

public slots:
    void launch(const QString &process);
    QString run(const QString &process);

};

#endif

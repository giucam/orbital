
#include <QProcess>

#include "processlauncher.h"

ProcessLauncher::ProcessLauncher(QObject *parent)
               : QObject(parent)
{
}

void ProcessLauncher::launch(const QString &process)
{
    QProcess::startDetached(process);
}

QString ProcessLauncher::run(const QString &process)
{
    QProcess::startDetached(process);
}


#include <stdlib.h>

#include <QApplication>

#include "client.h"

int main(int argc, char *argv[])
{
    // Force Wayland platform plugin
    setenv("QT_QPA_PLATFORM", "wayland", 1);

    QApplication app(argc, argv);
    Client client;

    return app.exec();
}

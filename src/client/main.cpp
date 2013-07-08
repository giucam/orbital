
#include <stdlib.h>

#include <QApplication>

int main(int argc, char *argv[])
{
    // Force Wayland platform plugin
    setenv("QT_QPA_PLATFORM", "wayland", 1);

    QApplication app(argc, argv);

    // Create the shell
//     (void)DesktopShell::instance();

    return app.exec();
}

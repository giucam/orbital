
import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0

QtObject {

    property list<ShellItem> items: [
        Background {
            imageSource: "/home/giulio/Immagini/civetta/DSCF0470.JPG"
            imageFillMode: Image.PreserveAspectFit
        },
        Panel {
            Launcher {
                icon: "/usr/share/icons/default.kde4/32x32/apps/utilities-terminal.png"
                process: "/home/giulio/projects/wayland/weston/clients/weston-terminal"
            }
        },
        Overlay {}
    ]

}


import QtQuick 2.1
import QtQuick.Window 2.1
import Orbital 1.0

ShellUI {
    iconTheme: "oxygen"

    Background {
        color: "black"
        imageSource: "/home/giulio/Immagini/civetta/DSCF0470.JPG"
        imageFillMode: Image.PreserveAspectFit
    }

    Panel {
        Launcher {
            icon: "image://icon/utilities-terminal"
            process: "/home/giulio/projects/wayland/weston/clients/weston-terminal"
        }
        Launcher {
            icon: "image://icon/ark"
            process: "/home/giulio/projects/wayland/weston/clients/gears"
        }
        TaskBar {}
        Spacer {}
    }

    Overlay {}

}

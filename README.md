Orbital
=======

Orbital is a Wayland compositor and shell, using Qt5 and Weston.
The goal of the project is to build a simple yet flexible and good looking
Wayland desktop. It is not a full fledged DE but rather the analogue of a WM
in the X11 world, such as Awesome or Fluxbox.

## Dependencies
Orbital depends on two things: Weston and Qt 5.
Since it uses QtQuick 2 to draw the interface it will make use of OpenGL,
so it is advisable to use a decent graphics driver, otherwise the performance
will not be good.
Orbital currently needs a patched weston from [here](https://github.com/giucam/weston),
branch libweston. You also need QtWayland, which is shipped with Qt starting with the 5.4 version.
You do not need QtCompositor, the platform plugin is enough.
There are also some optional dependencies: currently Orbital can use Solid from
KDE Frameworks 5, ALSA and Logind, but it can also work without them, losing some
functionality. You can enable or disable these dependencies by passing some options
to cmake: `-Duse_alsa=ON/OFF`, `-Duse_logind=ON/OFF` and `-Duse_solid=ON/OFF`.

## Building Orbital
To compile Orbital run this commands from the repository root directory:
```sh
mkdir build
cd build
cmake ..
make
sudo make install
```


Orbital will be installed in the */usr/local* prefix, unless you specified
otherwise using the `CMAKE_INSTALL_PREFIX` variable:
```sh
cmake -DCMAKE_INSTALL_PREFIX=/my/prefix ..
```

## Running Orbital
Now you can just run *orbital*, to run it if you are inside an X or a Wayland
session. To start its own dedicated session run *orbital-launch* from a tty.

## Configuring Orbital
The first time you start Orbital it will load a default configuration. If you
save the configuration (by closing the config dialog or by going from edit mode
to normal mode) it will save the configuration file *orbital/orbital.conf*, in
`$XDG_CONFIG_HOME` or, if not set, in `$HOME/.config`. You can manually modify
the configuration file, but Orbital has (or will have) graphical tools
for configuring the environment.

You can see a screencast of some of Orbital functionalities at this link:
http://www.youtube.com/watch?v=bd1hguj2bPE

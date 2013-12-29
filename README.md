Orbital
=======

Orbital is a shell client for Wayland, made in Qt5, with the interface in
QtQuick 2. It uses [Nuclear](https://github.com/nuclide/nuclear), a Weston
shell plugin, as the compositor side but theoretically it could be made
working with any other Wayland compositor, as long as it implements the
necessary protocol.
Its goal is to produce a light and self-contained shell running on Wayland,
without many dependencies aside Nuclear and Qt.

## Dependencies
Orbital depends on two things: [Nuclear](https://github.com/nuclide/nuclear) and Qt 5.2.
Since it uses QtQuick 2 to draw the interface it will make use of OpenGL,
so it is advisable to use a decent graphics driver, otherwise the performance
will not be good.
You also need QtWayland, preferably from git (branch 'stable') as Qt does not
provide a Wayland backend by default. You do not need QtCompositor, the
platform plugin is enough.

## Building Orbital
To compile Orbital run this commands from the repository root directory:
```sh
mkdir build
cd build
cmake ..
make
sudo make install
```


If *cmake* fails with the error `package 'nuclear' not found` but you do have
[Nuclear](https://github.com/nuclide/nuclear) installed you may need to set
the `PKG_CONFIG_PATH` environment variable to the right path
(replacing *NUCLEAR_PREFIX* with the right path):
```sh
export PKG_CONFIG_PATH=NUCLEAR_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH
cmake ..
```


Orbital will be installed in the */usr/local* prefix, unless you specified
otherwise using the `CMAKE_INSTALL_PREFIX` variable:
```sh
cmake -DCMAKE_INSTALL_PREFIX=/my/prefix ..
```

## Running Orbital
Now you can just run *orbital*, to run it. If you run it inside an X session
it will run in windowed mode, otherwise running it from a tty will start its
own dedicated session.

## Configuring Orbital
The first time you start Orbital it will load a default configuration. If you
save the configuration (by closing the config dialog or by going from edit mode
to normal mode) it will save the configuration file *orbital/orbital.conf*, in
`$XDG_CONFIG_HOME` or, if not set, in `$HOME/.config`. You can manually modify
the configuration file, but Orbital has (or will have) graphical tools
for configuring the environment.

You can see a screencast of some of Orbital functionalities at this link:
http://www.youtube.com/watch?v=bd1hguj2bPE

# Bonsai

Bonsai is a simple Wayland compositor focusing on out-of-the-box user experience.
It includes support for screenshots, screensharing and touchpad gestures without
any extra configuration required.

## Description

The aim of this project is to solve some issues that tend to arise when using Wayland
in combination with screencasting/screenshots and touchpads. As a laptop user I also
wanted to have support for touchpad gestures. Even though the major desktop environments
on Linux now include these features, this is still a fun project for me, just to see
what can be built. The compositor is based on the [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
library, meaning a rock-solid foundation to build on.

## Getting Started

### Build dependencies

* `meson`
* `wlroots` (currently based on commit [972a5cdf](https://gitlab.freedesktop.org/wlroots/wlroots/-/commit/972a5cdf7a9701a266119d60da48624ca8ebf703) from Jul 21st, support for v0.16 will be added upon release)
* `libinput`
* `libevdev`
* `libudev`
* `xkbcommon`
* `glesv2`
* `libdrm`
* `wayland-server`
* `wayland-client`
* `wayland-cursor`
* `wayland-egl`
* `wayland-protocols`
* `xdg-desktop-portal`
* `pixman-1`
* `cairo`

### Runtime dependencies

* `swaybg` (wallpaper support)
* `swaylock` (screen locking support)
* `bemenu` (run menu)
* `waybar` (status bar)
* `slurp` (screenshot support)
* `grim` (screenshot support)
* `wl-clipboard` (screenshot & clipboard)
* `brightnessctl` (brightness control via waybar)

### Using the Vagrant development environment
TODO: Finish this section

### Building and installing

* Clone the repo

```bash
git clone https://github.com/vilfa/bonsai.git
cd bonsai
```

* Install build and runtime dependencies

* Create Meson build directory (Meson will let you know about missing stuff at this point)

```bash
meson builddir
```

* Compile the project

```bash
meson compile -C builddir
```

* Install the project

```bash
meson install -C builddir
```

It is also possible to build and install Bonsai from a distro package (currently,
only an Arch Linux package spec is provided). In case you would like to install it
from a package, follow the steps below.

* Move to the `package/arch` directory

```bash
cd package/arch
```

* Build and install the package

```bash
makepkg -si
```

* You can also build and install separately

```bash
makepkg -s
pacman -U bonsai-0.1.0.dev-1-x86_64.pkg.tar.zst
```

### Running

* After install a `bonsai.desktop` file will be placed in your `/usr/share/wayland-sessions` directory.
This file allows you to start the desktop session straight from the display manager,
simply by selecting the 'Bonsai (Wayland)' session on your login screen.
* Another option is to run Bonsai as a client of main compositor. This option will
start Bonsai in a window, allowing you to give it a test drive. You can start it simply
by opening a shell and typing

```bash
bonsai
```

or if for some reason `/usr/bin` is not in your system PATH

```bash
/usr/bin/bonsai
```

### Keyboard shortcuts

* `Print` -> screenshot all outputs
* `Escape` -> un-fullscreen if fulscreen
* `F11` -> fullscreen focused view
* `Ctrl`+`Print` -> screenshot active output
* `Alt`+`Tab` -> cycle views
* `Super`+`L` -> lock session
* `Super`+`D` -> bemenu
* `Super`+`Return` -> term
* `Super`+`Up` -> maximize active
* `Super`+`Down` -> unmaximize active
* `Super`+`Left` -> tile left/untile
* `Super`+`Right` -> tile right/untile
* `Super`+`X` -> hide all workspace views
* `Super`+`Z` -> show all workspace views
* `Ctrl`+`Alt`+`Right` -> next workspace
* `Ctrl`+`Alt`+`Left` -> previous workspace
* `Ctrl`+`Shift`+`Print` -> screenshot selection
* `Super`+`Shift`+`Q` -> exit

## Help

If you would like to customize settings for your install, you can do so via a
config file. The following configuration files are recognized by Bonsai when
installing from source (in order of search):

* `.config/bonsai`
* `.config/bonsai/config`
* `/usr/local/etc/bonsai`
* `/usr/local/etc/bonsai/config`

If you are installing Bonsai from a package, the system-wide config locations will
be:

* `/etc/bonsai`
* `/etc/bonsai/config`

The default configuration file is located at `/etc/bonsai/config`
or `/usr/local/etc/bonsai/config` if installing with Meson with the default prefix.
You can use this file as a template, copy it to your user configuration folder,
and customize it as you see fit. The configuration file is documented in the comments
that it contains. You might find the following command handy for looking up the
device names to use in the configuration file:

```bash
sudo libinput list-devices
```

## Authors

[Luka V.](mailto:luka.vilfan@protonmail.com)

## Version History

* [0.1.0-dev](https://github.com/vilfa/bonsai/releases/tag/v0.1.0-dev)
  * Initial development release

## License

[MIT](https://github.com/vilfa/bonsai/blob/master/LICENSE)

## Acknowledgments

* [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
* [sway](https://github.com/swaywm/sway)

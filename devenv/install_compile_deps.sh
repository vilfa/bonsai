#!/usr/bin/env bash

install() {
    pacman -S --noconfirm $@
}

concat() {
    local -n in=$1
    for dep in ${in[@]}; do
        deps="$deps $dep"
    done
    echo $deps
}

dependencies=(
    base-devel
    gdb
    git
    meson
    libinput
    libevdev
    systemd-libs
    libxkbcommon
    mesa
    vulkan-headers
    libdrm
    wayland
    wayland-utils
    wayland-protocols
    xdg-desktop-portal
    xdg-desktop-portal-wlr
    pixman
    cairo
    seatd
    xcb-util
    xcb-util-renderutil
    xcb-util-wm
    xcb-util-errors
    xorg-xwayland
    glslang
    ffmpeg
    rsync
)

install $(concat dependencies)

install_wlroots_latest() {
    git clone https://gitlab.freedesktop.org/wlroots/wlroots.git /root/wlroots
    cd /root/wlroots
    meson builddir
    meson compile -C builddir
    meson install -C builddir
}

install_wlroots_latest

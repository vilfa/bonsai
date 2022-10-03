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
    swaybg
    swaylock
    bemenu-wayland
    waybar
    slurp
    grim
    wl-clipboard
    brightnessctl
)

install $(concat dependencies)

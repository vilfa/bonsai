#!/usr/bin/env bash

# This is meant to be run by Meson.

### Logging utils

info() {
    echo "[INFO] $1"
}

warn() {
    echo "[WARN] $1"
}

err() {
    echo "[ERROR] $1"
}

### Find configuration home

CONFDIR=""

if [[ -z "$XDG_CONFIG_HOME" ]]; then
    warn "XDG_CONFIG_HOME not found."
    info "Trying for HOME."
    if [[ -z "$HOME" ]]; then
        warn "HOME not found."
        err "Configuration directory not found. Exiting."
        exit 1
    else
        CONFDIR="$HOME/.config"
        info "Set config install directory to '$CONFDIR'."
    fi
else
    CONFDIR="$XDG_CONFIG_HOME"
    info "Set config install directory to '$CONFDIR'."
fi

info "Installing configs..."

cp -rv "$SRCDIR/extern/bonsai" "$CONFDIR"
[[ $? ]] && err "Error installing config. Exiting." && exit 1

cp -rv "$SRCDIR/extern/swaylock" "$CONFDIR"
[[ $? ]] && err "Error installing config. Exiting." && exit 1

cp -rv "$SRCDIR/extern/waybar" "$CONFDIR"
[[ $? ]] && err "Error installing config. Exiting." && exit 1

info "Installed configs."
info "Done."

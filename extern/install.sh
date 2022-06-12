#!/usr/bin/env bash

# This is meant to be run by Meson when invoking install.

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

### Install configs (swaylock, waybar, xdg-desktop-portal-wlr)

info "Installing configs..."

# cp -rv "$SRCDIR/extern/bonsai" "$CONFDIR"
cp -rv "$SRCDIR/extern/swaylock" "$CONFDIR"
cp -rv "$SRCDIR/extern/waybar" "$CONFDIR"
cp -rv "$SRCDIR/extern/xdg-desktop-portal-wlr" "$CONFDIR"

info "Installed configs."
info "Done."

#!/usr/bin/env zsh

### Logging utils

declare -A COLORS

COLORS=(
    [blue]="\033[1;34m"
    [yellow]="\033[1;33m"
    [red]="\033[1;31m"
    [nil]="\033[0m"
)

print_info() {
    echo "${COLORS[blue]}Info: $1${COLORS[nil]}"
}

print_warn() {
    echo "${COLORS[yellow]}Warn: $1${COLORS[nil]}"
}

print_err() {
    echo "${COLORS[red]}Err: $1${COLORS[nil]}"
}

### Project location utils

project_dir() {
    wd=${PWD##*/}
    case $wd in
    "bonsai")
        echo "$(realpath -s $PWD)"
        return
        ;;
    "remote")
        echo "$(realpath -s $PWD/..)"
        return
        ;;
    *)
        print_err "Please run from the project_root or project_root/remote directory"
        exit 1
        ;;
    esac
}

### Prefs location

prefs_loc() {
    wd=${PWD##*/}
    case $wd in
    "bonsai")
        echo "$(realpath -s $PWD/remote/prefs.sh)"
        return
        ;;
    "remote")
        echo "$(realpath -s $PWD/prefs.sh)"
        return
        ;;
    *)
        print_err "Please run from the project_root or project_root/remote directory"
        exit 1
        ;;
    esac
}

### Check working directory

CWD=$(project_dir)
PREFS=$(prefs_loc)

### Setup preferences

source "$PREFS"

### Setup helpers

generate_keys() {
    print_info "Generate ssh key ${VM_1[priv_key]}"
    ssh-keygen -t rsa -b 4096 -f "${VM_1[priv_key]}"
    print_info "Generate ssh key ${VM_2[priv_key]}"
    ssh-keygen -t rsa -b 4096 -f "${VM_2[priv_key]}"
}

setup_keys() {
    print_info "Install ssh key ${VM_1[priv_key]} to ${VM_1[user]}@${VM_1[host]}"
    ssh-copy-id -i "${VM_1[priv_key]}" "${VM_1[user]}@${VM_1[host]}"
    print_info "Install ssh key ${VM_2[priv_key]} to ${VM_2[user]}@${VM_2[host]}"
    ssh-copy-id -i "${VM_2[priv_key]}" "${VM_2[user]}@${VM_2[host]}"
}

### Setup

generate_keys
setup_keys

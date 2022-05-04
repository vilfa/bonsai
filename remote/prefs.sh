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
        print_err "Please run from the project or remote directory"
        exit 1
        ;;
    esac
}

### Check working directory

CWD=$(project_dir)

### ! REMOTE HOST PREFERENCES

declare -A VM_1
declare -A VM_2

VM_1=(
    [user]=user
    [host]=192.168.122.100
    [priv_key]="$CWD/remote/keys/id_one"
    [url]="${VM_1[user]}@${VM_1[host]}"
    [share]="$HOME/kvmshare/arch"
)

VM_2=(
    [user]=user
    [host]=192.168.122.200
    [priv_key]="$CWD/remote/keys/id_two"
    [url]="${VM_2[user]}@${VM_2[host]}"
    [share]="$HOME/kvmshare/ubuntu"
)

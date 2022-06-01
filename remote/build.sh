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

### Build helpers

copy_source() {
    print_info "Copy source from $CWD to ${VM_1[share]}"
    rsync -vh --no-compress --progress --info=progress -r \
        "$CWD" "${VM_1[share]}" \
        --exclude=".*" --exclude="builddir"
    print_info "Copy source from $CWD to ${VM_2[share]}"
    rsync -vh --no-compress --progress --info=progress -r \
        "$CWD" "${VM_2[share]}" \
        --exclude=".*" --exclude="builddir"
}

exec_remote() {
    print_info "Exec remote 'ssh $1 -i $2 $BUILD_CMD'"
    ssh "$1" -i "$2" "${@:3}"
}

build_cmd() {
    cmd=""
    for ((i = 1; i <= ${#BUILD_SEQ[@]}; ++i)); do
        if ((i > 1)); then
            cmd+=" && ${BUILD_SEQ[i]}"
        else
            cmd+="${BUILD_SEQ[i]}"
        fi
    done
    echo "$cmd"
}

build_seq() {
    seq=""
    for ((i = 1; i <= ${#BUILD_SEQ[@]}; ++i)); do
        seq+="\t${BUILD_SEQ[i]}\n"
    done
    echo "$seq"
}

### ! BUILD SETUP

BUILD_SEQ=(
    "cd remote/bonsai"
    "meson setup --wipe builddir"
    "meson configure --clearcache builddir"
    "meson compile --clean -C builddir"
    "meson compile -C builddir"
)

BUILD_SEQ_PRETTY=$(build_seq)
BUILD_CMD=$(build_cmd)

# Build info
print_info "Build sequence:\n$BUILD_SEQ_PRETTY"
print_info "Build command: $BUILD_CMD"

# Copy source to remote
copy_source

# Remote compile
exec_remote "$VM_1[user]@$VM_1[host]" "$VM_1[priv_key]" $(build_cmd)
exec_remote "$VM_2[user]@$VM_2[host]" "$VM_2[priv_key]" $(build_cmd)

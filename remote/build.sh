#!/usr/bin/env zsh

### Setup preferences

source ./prefs.sh

### Build helpers

copy_source() {
    print_info "Copy source from $CWD to ${VM_1[share]}"
    rsync -avh --no-compress --progress --info=progress -r \
        "$CWD" "${VM_1[share]}" \
        --exclude=".*" --exclude="builddir"
    print_info "Copy source from $CWD to ${VM_2[share]}"
    rsync -avh --no-compress --progress --info=progress -r \
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
    # "meson setup --wipe builddir"
    "meson setup builddir"
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

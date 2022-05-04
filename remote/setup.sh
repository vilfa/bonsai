#!/usr/bin/env zsh

### Setup preferences

source ./prefs.sh

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

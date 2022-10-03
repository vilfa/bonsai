#!/usr/bin/env bash

[ ! -d /home/vagrant/bonsai ] && echo "Run prepare_* first." && exit 1
rsync -vh --no-compress --progress --info=progress -r \
    /vagrant_data/* /home/vagrant/bonsai \
    --exclude=".*" --exclude="builddir"

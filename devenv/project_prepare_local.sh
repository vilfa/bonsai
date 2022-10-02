#!/usr/bin/env bash

[ -d /home/vagrant/bonsai ] && rm -rf /home/vagrant/bonsai
mkdir /home/vagrant/bonsai && cd /home/vagrant/bonsai
rsync -vh --no-compress --progress --info=progress -r \
    /vagrant_data/* /home/vagrant/bonsai \
    --exclude=".*" --exclude="builddir"

cd /home/vagrant/bonsai
[ ! -d builddir ] && PKG_CONFIG_PATH=/usr/local/lib/pkgconfig meson builddir/

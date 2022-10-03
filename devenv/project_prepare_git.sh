#!/usr/bin/env bash

[ ! -d /home/vagrant/bonsai ] && git clone https://github.com/vilfa/bonsai.git /home/vagrant/bonsai
cd /home/vagrant/bonsai
[ ! -d builddir ] && PKG_CONFIG_PATH=/usr/local/lib/pkgconfig meson builddir/

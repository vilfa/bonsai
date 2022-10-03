#!/usr/bin/env bash

[ ! -d /home/vagrant/bonsai ] && echo "Run prepare_* first." && exit 1

cd /home/vagrant/bonsai
meson compile -C builddir/

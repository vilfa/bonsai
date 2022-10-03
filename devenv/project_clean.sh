#!/usr/bin/env bash

[ ! -d /home/vagrant/bonsai ] && echo "Nothing to clean." && exit 0

cd /home/vagrant/bonsai

[ ! -d builddir ] && echo "Nothing to clean." && exit 0

rm -rf builddir

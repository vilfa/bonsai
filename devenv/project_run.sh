#!/usr/bin/env bash

[ ! -d /home/vagrant/bonsai ] && echo "Run prepare_* first." && exit 1

cd /home/vagrant/bonsai

[ ! -f builddir/bonsai/bonsai ] && echo "Run build first." && exit 1

tty=$(who | grep tty)
if [[ "$tty" ]]; then
    tty="/dev/$(who | grep tty | tr -s ' ' | cut -d ' ' -f 2)"
    echo "Open graphical console is $tty."
    echo "Running bonsai in the open graphical session is currently unsupported."
    echo "Run it manually with `cd /home/vagrant/bonsai && ./builddir/bonsai/bonsai`."
    # TODO: Run bonsai in the open graphical console.
    # setsid bash -c "export XDG_SEAT=seat0 && exec /home/vagrant/bonsai/builddir/bonsai/bonsai <> $tty >&0 2>&1"
    # sudo openvt -svfc 1 -- bash -c "export XDG_SEAT=seat0 && exec /home/vagrant/bonsai/builddir/bonsai"
else
    echo "Login in the graphical session first (vagrant:vagrant)."
    exit 1
fi

#!/bin/sh

set -e

./bootstrap.sh
if test -z "$NOCONFIGURE"; then
    exec ./configure --enable-maintainer-mode --enable-preset-menu=preset_menu.lst "$@"
fi

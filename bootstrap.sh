#!/bin/sh

set -e

aclocal || exit 1
autoheader || exit 1
autoconf || exit 1
automake -W no-portability -a -c || exit 1

#!/bin/sh

cd "$MESON_DIST_ROOT"
./autogen.sh
rm -rf autom4te.cache

#!/bin/bash

set -euo pipefail

SRCDIR=$1
BUILDDIR=$2
MODULE_PLAYGROUND=$3

# TODO: meson allows only out of tree builds
if test "$SRCDIR" != "$BUILDDIR"; then
    rsync --recursive --times "$SRCDIR/$MODULE_PLAYGROUND/" "$MODULE_PLAYGROUND/"
fi

export MAKEFLAGS=${MAKEFLAGS-"-j$(nproc)"}
"${MAKE-make}" -C "$PWD/$MODULE_PLAYGROUND" modules

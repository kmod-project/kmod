#!/bin/bash

set -euo pipefail

SRCDIR=$1
BUILDDIR=$2
MODULE_PLAYGROUND=$3
# shellcheck disable=SC2034 # used by the Makefile
export MODULE_DIRECTORY=$4

# TODO: meson allows only out of tree builds
if test "$SRCDIR" != "$BUILDDIR"; then
    rm -rf "$MODULE_PLAYGROUND"
    mkdir -p "$(dirname "$MODULE_PLAYGROUND")"
    cp -r "$SRCDIR/$MODULE_PLAYGROUND" "$MODULE_PLAYGROUND"
fi

export MAKEFLAGS=${MAKEFLAGS-"-j$(nproc)"}
"${MAKE-make}" -C "$PWD/$MODULE_PLAYGROUND" modules

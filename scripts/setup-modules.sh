#!/bin/bash

set -euo pipefail

SRCDIR=$1
MODULE_PLAYGROUND=$2

mkdir -p "$MODULE_PLAYGROUND"
cp --archive "$SRCDIR/$MODULE_PLAYGROUND/"* "$MODULE_PLAYGROUND/"

export MAKEFLAGS=${MAKEFLAGS-"-j$(nproc)"}
"${MAKE-make}" -C "$PWD/$MODULE_PLAYGROUND" modules

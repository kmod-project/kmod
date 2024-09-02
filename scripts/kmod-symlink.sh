#!/bin/bash

set -euo pipefail

# Dummy wrapper script, since that's the meson way. Aka, there is no access to
# DESTDIR from within meson nor any other way to check/fetch it.
#
# For context read through https://github.com/mesonbuild/meson/issues/9

ln -sf kmod "$MESON_INSTALL_DESTDIR_PREFIX/$1"

#!/bin/sh
# SPDX-FileCopyrightText: 2026 Lucas De Marchi <demarchi@kernel.org>
# SPDX-License-Identifier: LGPL-2.1-or-later

set -eu

SCRIPT_DIR=$(CDPATH="" cd -- "$(dirname -- "$0")" && pwd)
WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

git clone --depth 1 --filter=blob:none --no-checkout --sparse \
    --branch linux-rolling-stable \
    https://github.com/gregkh/linux.git "$WORKDIR/linux"

git -C "$WORKDIR/linux" sparse-checkout set \
    tools/objtool \
    tools/scripts \
    tools/build \
    tools/lib \
    tools/include \
    tools/arch
git -C "$WORKDIR/linux" checkout

git -C "$WORKDIR/linux" apply \
    "$SCRIPT_DIR/0001-objtool-Use-AT_MINSIGSTKSZ-to-size-the-sigaltstack.patch"

make -C "$WORKDIR/linux/tools/objtool" -j"$(nproc)"

mkdir -p /usr/local/bin
install -m755 "$WORKDIR/linux/tools/objtool/objtool" /usr/local/bin/objtool

case ":$PATH:" in
*:/usr/local/bin:*) ;;
*) export PATH="/usr/local/bin:$PATH" ;;
esac

if [ -n "${GITHUB_PATH:-}" ]; then
    echo /usr/local/bin >> "$GITHUB_PATH"
fi

#!/bin/bash

set -e

MODULE_PLAYGROUND=$1
ROOTFS=$2

declare -A map
map=(
    ["test-depmod/search-order-simple/lib/modules/4.4.4/kernel/crypto/"]="mod-simple.ko"
    ["test-depmod/search-order-simple/lib/modules/4.4.4/updates/"]="mod-simple.ko"
    ["test-depmod/search-order-same-prefix/lib/modules/4.4.4/foo/"]="mod-simple.ko"
    ["test-depmod/search-order-same-prefix/lib/modules/4.4.4/foobar/"]="mod-simple.ko"
    ["test-depmod/detect-loop/lib/modules/4.4.4/kernel/mod-loop-a.ko"]="mod-loop-a.ko"
    ["test-depmod/detect-loop/lib/modules/4.4.4/kernel/mod-loop-b.ko"]="mod-loop-b.ko"
    ["test-depmod/detect-loop/lib/modules/4.4.4/kernel/mod-loop-c.ko"]="mod-loop-c.ko"
    ["test-depmod/detect-loop/lib/modules/4.4.4/kernel/mod-loop-d.ko"]="mod-loop-d.ko"
    ["test-depmod/detect-loop/lib/modules/4.4.4/kernel/mod-loop-e.ko"]="mod-loop-e.ko"
    ["test-dependencies/lib/modules/4.0.20-kmod/kernel/fs/foo/"]="mod-foo-b.ko"
    ["test-dependencies/lib/modules/4.0.20-kmod/kernel/"]="mod-foo-c.ko"
    ["test-dependencies/lib/modules/4.0.20-kmod/kernel/lib/"]="mod-foo-a.ko"
    ["test-dependencies/lib/modules/4.0.20-kmod/kernel/fs/"]="mod-foo.ko"
    ["test-init/"]="mod-simple.ko"
    ["test-remove/"]="mod-simple.ko"
    ["test-modprobe/show-depends/lib/modules/4.4.4/kernel/mod-loop-a.ko"]="mod-loop-a.ko"
    ["test-modprobe/show-depends/lib/modules/4.4.4/kernel/mod-loop-b.ko"]="mod-loop-b.ko"
    ["test-modprobe/show-depends/lib/modules/4.4.4/kernel/mod-simple.ko"]="mod-simple.ko"
    ["test-modprobe/softdep-loop/lib/modules/4.4.4/kernel/mod-loop-a.ko"]="mod-loop-a.ko"
    ["test-modprobe/softdep-loop/lib/modules/4.4.4/kernel/mod-loop-b.ko"]="mod-loop-b.ko"
    ["test-modprobe/install-cmd-loop/lib/modules/4.4.4/kernel/mod-loop-a.ko"]="mod-loop-a.ko"
    ["test-modprobe/install-cmd-loop/lib/modules/4.4.4/kernel/mod-loop-b.ko"]="mod-loop-b.ko"
    ["test-modprobe/force/lib/modules/4.4.4/kernel/"]="mod-simple.ko"
    ["test-modprobe/oldkernel/lib/modules/3.3.3/kernel/"]="mod-simple.ko"
    ["test-modprobe/oldkernel-force/lib/modules/3.3.3/kernel/"]="mod-simple.ko"
    ["test-modprobe/alias-to-none/lib/modules/4.4.4/kernel/"]="mod-simple.ko"
)

for k in ${!map[@]}; do
    dst=${ROOTFS}/$k
    src=${MODULE_PLAYGROUND}/${map[$k]}

    if test "${dst: -1}" = "/"; then
        install -d $dst
        install -t $dst $src
    else
        install -D $src $dst
    fi
done

#!/bin/bash

set -e

MODULE_PLAYGROUND=$1
ROOTFS=$2

declare -A map
map=(
    ["test-depmod/search-order-simple/lib/modules/4.4.4/kernel/crypto/"]="mod-simple.ko"
    ["test-depmod/search-order-simple/lib/modules/4.4.4/updates/"]="mod-simple.ko"
)

for k in ${!map[@]}; do
    dst=${ROOTFS}/$k
    src=${MODULE_PLAYGROUND}/${map[$k]}

    install -d $dst
    install -t $dst $src
done

#!/bin/bash

set -euo pipefail

for moddir in /usr/lib/modules /lib/modules; do
    if [[ -e "$moddir" ]]; then
        kernel=$(find "$moddir" -maxdepth 1 -mindepth 1 -type d -exec basename {} \;)
        break
    fi
done

# There should be one entry - the kernel we installed
if (( $(echo "$kernel" | wc -l) != 1 )); then
    echo >&2 "Error: exactly one kernel must be installed"
fi
echo "KDIR=$moddir/$kernel/build"

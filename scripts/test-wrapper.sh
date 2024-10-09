#!/bin/bash

set -euo pipefail

if [[ ${ASAN_OPTIONS-} || ${UBSAN_OPTIONS-} || ${MSAN_OPTIONS-} ]]; then
    LD_PRELOAD=$(gcc -print-file-name=libasan.so)
    # In some cases, like in Fedora, the file is a script which cannot be
    # preloaded. Attempt to extract the details from within.
    if grep -q INPUT "$LD_PRELOAD"; then
        input=$(sed -r -n "s/INPUT \( (.*) \)/\1/p" "$LD_PRELOAD")
        test -f "$input" && LD_PRELOAD="$input"
    fi
    export LD_PRELOAD
fi

exec "$@"

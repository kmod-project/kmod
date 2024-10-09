#!/bin/bash

set -euo pipefail

if [[ ${ASAN_OPTIONS-} || ${UBSAN_OPTIONS-} || ${MSAN_OPTIONS-} ]]; then
    if [[ $CC == *gcc* ]]; then
        LD_PRELOAD=$("$CC" -print-file-name=libasan.so)
    elif [[ $CC == *clang* ]]; then
        LD_PRELOAD=$("$CC" -print-file-name=libclang_rt.asan-x86_64.so)
    else
        echo "Unknown compiler CC=\"$CC\""
        exit 1
    fi
    export LD_PRELOAD
fi

exec "$@"

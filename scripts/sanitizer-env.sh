#!/bin/bash

# set -euo pipefail # don't set these, since this script is sourced

if [[ ${CC-} == *gcc* ]]; then
    OUR_PRELOAD=$("$CC" -print-file-name=libasan.so)
elif [[ ${CC-} == *clang* ]]; then
    OUR_PRELOAD=$("$CC" -print-file-name=libclang_rt.asan-x86_64.so)
else
    echo "Unknown compiler CC=\"${CC-}\" - gcc and clang are supported."
    echo "Assuming \"gcc\", manually set the variable and retry if needed."
    echo
    OUR_PRELOAD=$(gcc -print-file-name=libasan.so)
fi

if test -n "$OUR_PRELOAD"; then
    # In some cases, like in Fedora, the file is a script which cannot be
    # preloaded. Attempt to extract the details from within.
    if grep -q INPUT "$OUR_PRELOAD"; then
        input=$(sed -r -n "s/INPUT \( (.*) \)/\1/p" "$OUR_PRELOAD")
        test -f "$input" && OUR_PRELOAD="$input"
    fi

    LD_PRELOAD=${LD_PRELOAD+${LD_PRELOAD}:}$OUR_PRELOAD
    export LD_PRELOAD
    echo "LD_PRELOAD has been set to \"$LD_PRELOAD\"."
    echo "The sanitizer might report issues with ANY process you execute."
fi
unset OUR_PRELOAD

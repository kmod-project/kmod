#!/bin/bash

# set -euo pipefail # don't set these, since this script is sourced

if [[ ${CC-} == *gcc* ]]; then
    OUR_PRELOAD=$($CC -print-file-name=libasan.so)
elif [[ ${CC-} == *clang* ]]; then
    # With v19, the library lacks the CPU arch in its name
    OUR_PRELOAD=$($CC -print-file-name=libclang_rt.asan.so)
    if ! test -f "$OUR_PRELOAD"; then
       OUR_PRELOAD=$($CC -print-file-name=libclang_rt.asan-x86_64.so)
    fi
else
    cat <<- EOF >&2

    WARNING: Unknown compiler CC="${CC-}" - gcc and clang are supported.
    Assuming "gcc", manually set the variable and retry if needed.

EOF
    OUR_PRELOAD=$(gcc -print-file-name=libasan.so)
fi

if test -f "$OUR_PRELOAD"; then
    # In some cases, like in Fedora, the file is a script which cannot be
    # preloaded. Attempt to extract the details from within.
    if grep -q INPUT "$OUR_PRELOAD"; then
        input=$(sed -r -n "s/INPUT \( (.*) \)/\1/p" "$OUR_PRELOAD")
        test -f "$input" && OUR_PRELOAD="$input"
    fi

    LD_PRELOAD=${LD_PRELOAD+${LD_PRELOAD}:}$OUR_PRELOAD
    export LD_PRELOAD
    cat <<- EOF

    LD_PRELOAD has been set to "$LD_PRELOAD".
    The sanitizer might report issues with ANY process you execute.

EOF
else
    cat <<- EOF >&2

    WARNING: compiler returned non-existing library name "$OUR_PRELOAD".

    Make sure to install the relevant packages and ensure this script
    references the correct library name.

    LD_PRELOAD will NOT be set.

EOF
fi
unset OUR_PRELOAD

# TODO: add helper to validate only_bits and skip_test
# Allowed values are 32, 64 and help.

#
# Simple helper executing the given function for all bit combinations
#
for_each_config() {
    [[ -n $1 ]] && bits="$1" || bits="32 64"
    [[ -n $2 ]] && suffix="-$2" || suffix=""
    meson_cmd=$3

    for bit in ${bits[@]}; do
        echo "::group::$bit bit"
        builddir="builddir-$bit$suffix/"

        if [[ "$bit" == "32" ]]; then
            CC="$CC -m32" $meson_cmd "$builddir" "$bit"
        else
            $meson_cmd "$builddir" "$bit"
        fi

        echo "::endgroup::"
    done
}

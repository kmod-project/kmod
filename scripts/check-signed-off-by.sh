#!/bin/bash
# SPDX-FileCopyrightText: 2026 mahailiang <mahailiang@uniontech.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Check that every commit in <base>..<head> carries a Signed-off-by trailer
# matching its author.  Merge commits are skipped.

set -o pipefail

base=${1:?usage: $0 <base> [<head>]}
head=${2:-HEAD}

commits=$(git rev-list --no-merges "$base..$head") || exit 1

status=0
while IFS= read -r commit; do
    [[ -z $commit ]] && continue
    author=$(git show -s --format='%an <%ae>' "$commit")
    if ! git show -s --format=%B "$commit" |
        grep -Fiqx "Signed-off-by: $author"; then
        echo "::error::commit $commit is missing 'Signed-off-by: $author'"
        status=1
    fi
done <<< "$commits"

exit "$status"

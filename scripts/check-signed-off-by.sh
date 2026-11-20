#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# SPDX-FileCopyrightText: 2026 Polly Labs <pollychen.lab@gmail.com>

set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 BASE_SHA HEAD_SHA" >&2
    exit 2
fi

base_sha=$1
head_sha=$2
merge_base=$(git merge-base "$base_sha" "$head_sha")
commit_range="$merge_base..$head_sha"
commit_log=$(mktemp)
trap 'rm -f "$commit_log"' EXIT
git log --reverse -z \
    --format='%H%n%an <%ae>%n%(trailers:key=Signed-off-by,valueonly)' \
    "$commit_range" >"$commit_log"

if [[ ! -s "$commit_log" ]]; then
    echo "::error::No pull request commits found"
    exit 1
fi

check_commits() {
    local IFS=$'\n'
    local commit_data commit author trailer found

    while read -r -d '' commit_data; do
        {
            read -r commit
            read -r author
            printf 'Checking commit %s by %s\n' "$commit" "$author"

            found=0
            while read -r trailer; do
                if [[ "$author" == "$trailer" ]]; then
                    found=1
                    break
                fi
            done

            if ((found == 0)); then
                echo "::error::Commit $commit is missing a matching Signed-off-by trailer for $author"
                exit 1
            fi
        } <<<"$commit_data"
    done <"$commit_log"
}

check_commits

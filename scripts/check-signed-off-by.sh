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
commit_range="$base_sha..$head_sha"
if [[ "$base_sha" =~ ^0+$ ]]; then
    commit_range=$head_sha
fi

check_commits() {
    local IFS=$'\n'
    local commit_data commit author trailer found commit_count=0

    while read -r -d '' commit_data; do
        commit_count=$((commit_count + 1))
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
                return 1
            fi
        } <<<"$commit_data"
    done < <(git log --reverse -z \
        --format='%H%n%an <%ae>%n%(trailers:key=Signed-off-by,valueonly)' \
        "$commit_range")

    if ((commit_count == 0)); then
        echo "::error::No pull request commits found"
        return 1
    fi
}

check_commits

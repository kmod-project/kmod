#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "main.yml"
DEFAULT_SETUP_OS = REPO_ROOT / ".github" / "actions" / "setup-os"
DEFAULT_PREFIX = "localhost/kmod-ci"
DEFAULT_IMAGE_ENV = ["KDIR=any"]

MATRIX_EXPR_RE = re.compile(r"^\s*\$\{\{\s*matrix\.([A-Za-z_][A-Za-z0-9_-]*)\s*\}\}\s*$")

SETUP_COMMAND = r"""
set -eu

GITHUB_ACTION_PATH=/setup-os
GITHUB_ENV=/tmp/github-env
export GITHUB_ACTION_PATH GITHUB_ENV

. /etc/os-release
DISTRO="${ID:?}"
export DISTRO

echo "Distro: ${DISTRO}"
printf 'DISTRO=%s\n' "${DISTRO}" >> "${GITHUB_ENV}"

chmod +x "${GITHUB_ACTION_PATH}"/setup-*.sh
setup_script="${GITHUB_ACTION_PATH}/setup-${DISTRO}.sh"
if [ ! -x "${setup_script}" ]; then
    echo "Missing setup script: ${setup_script}" >&2
    exit 1
fi

"${setup_script}"
"""


@dataclass(frozen=True)
class ContainerImage:
    source: str
    local: str


def unique(items: list[str]) -> list[str]:
    seen: set[str] = set()
    values: list[str] = []

    for item in items:
        if item in seen:
            continue
        seen.add(item)
        values.append(item)

    return values


def strip_yaml_scalar(value: str) -> str | None:
    value = value.strip()
    if not value or value in {"|", ">"}:
        return None

    if value[0] == "'":
        end = value.find("'", 1)
        if end != -1:
            return value[1:end].replace("''", "'")

    if value[0] == '"':
        end = value.find('"', 1)
        if end != -1:
            return bytes(value[1:end], "utf-8").decode("unicode_escape")

    if " #" in value:
        value = value.split(" #", 1)[0].strip()

    return value or None


def resolve_container_value(value: Any, matrix_values: dict[str, list[str]]) -> list[str]:
    if not isinstance(value, str):
        return []

    value = value.strip()
    match = MATRIX_EXPR_RE.match(value)
    if match:
        return matrix_values.get(match.group(1), [])

    if "${{" in value:
        print(f"warning: unresolved container expression: {value}", file=sys.stderr)
        return []

    return [value]


def collect_matrix_values(matrix: Any) -> dict[str, list[str]]:
    values: dict[str, list[str]] = {}

    if not isinstance(matrix, dict):
        return values

    for key, value in matrix.items():
        if key == "include":
            continue

        if isinstance(value, list):
            for item in value:
                if isinstance(item, str):
                    values.setdefault(key, []).append(item)
                elif isinstance(item, dict) and isinstance(item.get("image"), str):
                    values.setdefault(key, []).append(item["image"])
        elif isinstance(value, str):
            values.setdefault(key, []).append(value)

    include = matrix.get("include")
    if isinstance(include, list):
        for entry in include:
            if not isinstance(entry, dict):
                continue

            for key, value in entry.items():
                if isinstance(value, str):
                    values.setdefault(key, []).append(value)

    return {key: unique(value) for key, value in values.items()}


def collect_job_containers(job: Any) -> list[str]:
    if not isinstance(job, dict):
        return []

    matrix = job.get("strategy", {}).get("matrix", {})
    matrix_values = collect_matrix_values(matrix)
    images: list[str] = []

    container = job.get("container")
    if isinstance(container, str):
        images.extend(resolve_container_value(container, matrix_values))
    elif isinstance(container, dict):
        images.extend(resolve_container_value(container.get("image"), matrix_values))

    # GitHub workflows commonly model the image as matrix.container and then
    # reference it from jobs.<job>.container.image. Keep this direct scan so a
    # matrix edit is enough for local updates too.
    images.extend(matrix_values.get("container", []))

    return images


def containers_from_yaml(data: Any) -> list[str]:
    jobs = data.get("jobs") if isinstance(data, dict) else None
    if not isinstance(jobs, dict):
        return []

    images: list[str] = []
    for job in jobs.values():
        images.extend(collect_job_containers(job))

    return unique(images)


def containers_from_text(text: str) -> list[str]:
    images: list[str] = []
    container_block_indent: int | None = None

    for line in text.splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue

        indent = len(line) - len(line.lstrip())

        if container_block_indent is not None and indent <= container_block_indent:
            container_block_indent = None

        if container_block_indent is not None:
            match = re.match(r"\s*image:\s*(.+)$", line)
            if match:
                value = strip_yaml_scalar(match.group(1))
                if value and "${{" not in value:
                    images.append(value)
                continue

        match = re.match(r"\s*-\s+container:\s*(.+)$", line)
        if match:
            value = strip_yaml_scalar(match.group(1))
            if value and "${{" not in value:
                images.append(value)
            continue

        match = re.match(r"\s*container:\s*(.*)$", line)
        if match:
            value = strip_yaml_scalar(match.group(1))
            if value:
                if "${{" not in value:
                    images.append(value)
            else:
                container_block_indent = indent

    return unique(images)


def parse_workflow(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")

    try:
        import yaml  # type: ignore[import-not-found]
    except ImportError:
        return containers_from_text(text)

    data = yaml.safe_load(text)
    images = containers_from_yaml(data)
    if images:
        return images

    return containers_from_text(text)


def split_image_reference(image: str) -> tuple[str, str]:
    if "://" in image:
        image = image.split("://", 1)[1]

    if "@" in image:
        name, digest = image.split("@", 1)
        return name, digest.replace(":", "-")

    last_colon = image.rfind(":")
    last_slash = image.rfind("/")
    if last_colon > last_slash:
        return image[:last_colon], image[last_colon + 1 :]

    return image, "latest"


def sanitize_repository(value: str) -> str:
    value = re.sub(r"[^a-z0-9._/-]+", "-", value.lower())
    value = re.sub(r"/+", "/", value)
    return value.strip("/-.") or "image"


def sanitize_tag(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9_.-]+", "-", value)
    value = value.strip(".-") or "latest"
    if not re.match(r"^[A-Za-z0-9_]", value):
        value = f"t{value}"
    return value[:128]


def tag_with_suffix(tag: str, suffix: str) -> str:
    tag = sanitize_tag(tag)
    suffix = sanitize_tag(suffix)
    max_tag_len = 128 - len(suffix) - 1

    if max_tag_len < 1:
        raise SystemExit(f"date suffix is too long for a container tag: {suffix}")

    tag = tag[:max_tag_len].rstrip(".-") or "latest"
    return f"{tag}-{suffix}"


def local_tag(image: str, prefix: str, date_suffix: str) -> str:
    repository, tag = split_image_reference(image)
    repository = sanitize_repository(repository)
    tag = tag_with_suffix(tag, date_suffix)
    return f"{prefix.rstrip('/')}/{repository}:{tag}"


def container_images(workflow: Path, prefix: str, date_suffix: str) -> list[ContainerImage]:
    return [
        ContainerImage(source=image, local=local_tag(image, prefix, date_suffix))
        for image in parse_workflow(workflow)
    ]


def run(cmd: list[str], *, capture_stdout: bool = False, dry_run: bool = False) -> subprocess.CompletedProcess[str] | None:
    print("+ " + " ".join(shlex.quote(arg) for arg in cmd), file=sys.stderr)
    if dry_run:
        return None

    return subprocess.run(
        cmd,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture_stdout else None,
    )


def check_podman() -> None:
    if shutil.which("podman") is None:
        raise SystemExit("podman was not found in PATH")


def container_name(image: str) -> str:
    digest = hashlib.sha256(image.encode("utf-8")).hexdigest()[:12]
    return f"kmod-ci-setup-{digest}-{os.getpid()}"


def update_image(
    image: ContainerImage,
    *,
    setup_os: Path,
    pull: bool,
    dry_run: bool,
    keep_failed: bool,
) -> None:
    name = container_name(image.source)
    created = False

    if pull:
        run(["podman", "pull", image.source], dry_run=dry_run)

    try:
        run(
            [
                "podman",
                "create",
                "--name",
                name,
                image.source,
                "/bin/sh",
                "-c",
                SETUP_COMMAND,
            ],
            dry_run=dry_run,
        )
        created = True

        run(["podman", "cp", str(setup_os), f"{name}:/setup-os"], dry_run=dry_run)
        run(["podman", "start", "--attach", name], dry_run=dry_run)

        commit_cmd = ["podman", "commit"]
        for env in DEFAULT_IMAGE_ENV:
            commit_cmd.extend(["--change", f"ENV {env}"])
        commit_cmd.append(name)

        result = run(commit_cmd, capture_stdout=True, dry_run=dry_run)
        image_id = "<dry-run>"
        if result is not None:
            image_id = result.stdout.strip()

        run(["podman", "tag", image_id, image.local], dry_run=dry_run)
        print(f"{image.source} -> {image.local}")
    except subprocess.CalledProcessError:
        if keep_failed:
            print(f"container left for inspection: {name}", file=sys.stderr)
        raise
    finally:
        if created and not keep_failed:
            run(["podman", "rm", "-f", name], dry_run=dry_run)


def parse_args() -> argparse.Namespace:
    today = dt.date.today().isoformat()

    parser = argparse.ArgumentParser(description="Manage local CI container images.")
    parser.add_argument(
        "--workflow",
        type=Path,
        default=DEFAULT_WORKFLOW,
        help=f"workflow to parse (default: {DEFAULT_WORKFLOW})",
    )
    parser.add_argument(
        "--prefix",
        default=DEFAULT_PREFIX,
        help=f"local image prefix (default: {DEFAULT_PREFIX})",
    )
    parser.add_argument(
        "--date",
        default=today,
        help=f"date suffix for local tags (default: {today})",
    )

    subparsers = parser.add_subparsers(dest="verb", required=True)

    subparsers.add_parser("list", help="list source images and generated local tags")

    update = subparsers.add_parser("update", help="create local images with setup-os applied")
    update.add_argument(
        "--no-pull",
        action="store_true",
        help="do not pull source images before creating containers",
    )
    update.add_argument(
        "--dry-run",
        action="store_true",
        help="print podman commands without running them",
    )
    update.add_argument(
        "--keep-failed",
        action="store_true",
        help="keep failed setup containers for inspection",
    )
    update.add_argument(
        "image",
        nargs="*",
        help="optional source image filter; defaults to all workflow containers",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    images = container_images(args.workflow, args.prefix, args.date)

    if args.verb == "list":
        for image in images:
            print(f"{image.source} -> {image.local}")
        return 0

    if not DEFAULT_SETUP_OS.exists():
        raise SystemExit(f"missing setup action: {DEFAULT_SETUP_OS}")

    requested = set(args.image)
    if requested:
        known = {image.source for image in images}
        unknown = requested - known
        if unknown:
            values = ", ".join(sorted(unknown))
            raise SystemExit(f"unknown workflow container(s): {values}")

        images = [image for image in images if image.source in requested]

    if not args.dry_run:
        check_podman()

    for image in images:
        update_image(
            image,
            setup_os=DEFAULT_SETUP_OS,
            pull=not args.no_pull,
            dry_run=args.dry_run,
            keep_failed=args.keep_failed,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

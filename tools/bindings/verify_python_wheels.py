#!/usr/bin/env python3
"""Verify that a cibuildwheel artifact set covers every supported platform."""

from __future__ import annotations

import argparse
from pathlib import Path


PYTHON_TAGS = ("cp310", "cp311", "cp312", "cp313")
PLATFORM_SUFFIXES = {
    "macos-arm64": "_arm64",
    "macos-x86_64": "_x86_64",
    "linux-x86_64": "_x86_64",
    "linux-arm64": "_aarch64",
    "windows-x86_64": "win_amd64",
}


def wheel_platform(filename: str) -> str:
    return filename.removesuffix(".whl").rsplit("-", 1)[-1]


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    for platform, suffix in PLATFORM_SUFFIXES.items():
        artifact = root / f"wheels-{platform}"
        wheels = sorted(artifact.glob("yunlink-*.whl"))
        if not wheels:
            errors.append(f"{platform}: no YunLink wheels found in {artifact}")
            continue
        for python_tag in PYTHON_TAGS:
            matches = [wheel for wheel in wheels if f"-{python_tag}-{python_tag}-" in wheel.name]
            if len(matches) != 1:
                errors.append(
                    f"{platform}: expected one {python_tag} wheel, found {len(matches)}"
                )
                continue
            platform_tag = wheel_platform(matches[0].name)
            if not platform_tag.endswith(suffix):
                errors.append(
                    f"{platform}: {matches[0].name} has unexpected platform tag"
                )
            if platform.startswith("linux-") and "manylinux" not in platform_tag:
                errors.append(f"{platform}: {matches[0].name} is not a manylinux wheel")
            if platform.startswith("macos-") and not platform_tag.startswith("macosx_"):
                errors.append(f"{platform}: {matches[0].name} is not a macOS wheel")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("wheelhouse", type=Path)
    args = parser.parse_args()
    errors = validate(args.wheelhouse)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1
    print("Wheel matrix verified: 5 platforms x 4 CPython versions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

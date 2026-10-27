#!/usr/bin/env python3
"""Verify that a cibuildwheel artifact set covers every supported platform."""

from __future__ import annotations

import argparse
from email.parser import Parser
import os
from pathlib import Path
import re
import tomllib
from zipfile import ZipFile


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


def validate(root: Path, version: str) -> list[str]:
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
        for wheel in wheels:
            if wheel.name.split("-")[1] != version:
                errors.append(f"{wheel.name}: expected version {version}")
            with ZipFile(wheel) as archive:
                metadata = [name for name in archive.namelist() if name.endswith(".dist-info/METADATA")]
                if len(metadata) != 1:
                    errors.append(f"{wheel.name}: expected one METADATA file")
                    continue
                package = Parser().parsestr(archive.read(metadata[0]).decode("utf-8"))
                if package["Name"] != "yunlink" or package["Version"] != version:
                    errors.append(f"{wheel.name}: package metadata does not match yunlink {version}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("wheelhouse", type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[2]
    project = tomllib.loads((source / "bindings/python/pyproject.toml").read_text(encoding="utf-8"))
    version = project["project"]["version"]
    errors = validate(args.wheelhouse, version)
    cmake = (source / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(rf"project\(yunlink VERSION {re.escape(version)}\s", cmake):
        errors.append(f"CMake project version does not match Python package {version}")
    if os.environ.get("GITHUB_REF_TYPE") == "tag":
        if os.environ.get("GITHUB_REF_NAME") != f"v{version}":
            errors.append(f"release tag must be v{version}")
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1
    print("Wheel matrix verified: 5 platforms x 4 CPython versions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

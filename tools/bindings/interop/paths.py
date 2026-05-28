from __future__ import annotations

from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[3]
BUILD_DIR = ROOT_DIR / "build" / "ninja-debug"
RUST_EXAMPLE_DIR = ROOT_DIR / "bindings" / "rust" / "target" / "debug" / "examples"
PYTHON_BIN = ROOT_DIR / ".venv" / "bin" / "python"
RUST_BUILD_DIR = next(
    (ROOT_DIR / path)
    for path in sorted(
        Path("bindings/rust/target/debug/build").glob("yunlink-sys-*/out/cmake-build")
    )
)


def scenario_path(role: str, name: str) -> Path:
    return ROOT_DIR / "tools" / "bindings" / "scenarios" / role / name

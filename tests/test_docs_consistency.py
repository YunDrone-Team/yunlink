#!/usr/bin/env python3
"""Consistency checks for testing docs, matrix, and referenced local paths."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]

DOCS_TO_SCAN = [
    ROOT_DIR / "README.md",
    ROOT_DIR / "docs" / "bindings" / "test-matrix.md",
    ROOT_DIR / "docs" / "bindings" / "test-world-map.md",
    ROOT_DIR / "docs" / "bindings" / "testing-todo-checklist.md",
    ROOT_DIR / "docs" / "protocol" / "implementation-status.md",
    ROOT_DIR / "docs" / "protocol" / "scenario-walkthroughs.md",
]

EXPECTED_EXISTING_REFERENCES = [
    "docs/bindings/test-matrix.md",
    "tests/core/test_protocol_corruption.cpp",
    "tests/runtime/test_state_plane_semantics.cpp",
    "tests/runtime/test_command_result_edges.cpp",
]

EXPECTED_STATES = {
    "protocol version mismatch": [
        "covered-in-repo",
        "test_runtime_version_rejection",
    ],
    "tcp connect / reconnect / duplicate connect": [
        "[x] 覆盖 TCP connect / reconnect / duplicate connect。",
    ],
    "group target precise match": [
        "TargetSelector::matches()` 已按 endpoint `group_ids` 精确匹配 group_id",
        "group target matched wrong group id",
    ],
}


def markdown_local_links(text: str) -> list[str]:
    matches = re.findall(r"\[[^\]]+\]\(([^)]+)\)", text)
    local: list[str] = []
    for match in matches:
        if match.startswith(("http://", "https://", "mailto:")):
            continue
        if match.startswith("../") or match.startswith("./") or match.endswith(".md") or match.endswith(".cpp") or match.endswith(".py") or match.endswith(".yaml"):
            local.append(match)
    return local


class DocsConsistencyTests(unittest.TestCase):
    def test_referenced_paths_exist(self) -> None:
        for doc in DOCS_TO_SCAN:
            text = doc.read_text(encoding="utf-8")
            for link in markdown_local_links(text):
                target = (doc.parent / link).resolve()
                self.assertTrue(target.exists(), f"missing referenced path {link} from {doc}")

    def test_expected_repo_local_references_exist(self) -> None:
        for relpath in EXPECTED_EXISTING_REFERENCES:
            self.assertTrue((ROOT_DIR / relpath).exists(), relpath)

    def test_docs_no_longer_reference_missing_external_bridge_paths(self) -> None:
        for doc in DOCS_TO_SCAN:
            text = doc.read_text(encoding="utf-8")
            self.assertNotIn("sunray_v2/communication/sunray_yunlink_bridge", text, doc)

    def test_matrix_and_drifted_docs_are_aligned(self) -> None:
        matrix = (ROOT_DIR / "docs" / "bindings" / "test-matrix.md").read_text(encoding="utf-8")
        todo = (
            ROOT_DIR / "docs" / "bindings" / "testing-todo-checklist.md"
        ).read_text(encoding="utf-8")
        world = (ROOT_DIR / "docs" / "bindings" / "test-world-map.md").read_text(
            encoding="utf-8"
        )
        impl = (ROOT_DIR / "docs" / "protocol" / "implementation-status.md").read_text(
            encoding="utf-8"
        )
        walkthrough = (
            ROOT_DIR / "docs" / "protocol" / "scenario-walkthroughs.md"
        ).read_text(encoding="utf-8")
        target_selector = (
            ROOT_DIR / "tests" / "core" / "test_target_selector.cpp"
        ).read_text(encoding="utf-8")

        self.assertIn("protocol / header / schema mismatch", matrix)
        self.assertIn("test_runtime_version_rejection", matrix)
        self.assertIn("[x] 覆盖 protocol version mismatch 的拒绝路径。", todo)
        self.assertIn("[x] protocol version mismatch 的 runtime 拒绝路径。", world)
        self.assertIn("test_runtime_version_rejection", impl)

        self.assertIn(
            "TargetSelector::matches()` 已按 endpoint `group_ids` 精确匹配 group_id",
            impl,
        )
        self.assertIn(
            "当前 `TargetSelector::matches()` 已按 endpoint `group_ids` 精确匹配 `group_id`",
            walkthrough,
        )
        self.assertIn("group target matched wrong group id", target_selector)

    def test_test_matrix_uses_only_allowed_status_and_gate_values(self) -> None:
        matrix = (ROOT_DIR / "docs" / "bindings" / "test-matrix.md").read_text(encoding="utf-8")
        allowed_status = {"`covered-in-repo`", "`implemented-now`", "`blocked-external`"}
        allowed_gate = {
            "`pr`",
            "`ci`",
            "`nightly-local`",
            "`manual-external`",
            "`release-external`",
        }
        for line in matrix.splitlines():
            if not line.startswith("| ") or "---" in line:
                continue
            cells = [cell.strip() for cell in line.strip().split("|")[1:-1]]
            if len(cells) != 6:
                continue
            if cells == ["domain", "scenario", "status", "evidence", "gate_tier", "env_requirement"]:
                continue
            self.assertIn(cells[2], allowed_status, line)
            self.assertIn(cells[4], allowed_gate, line)


if __name__ == "__main__":
    unittest.main()

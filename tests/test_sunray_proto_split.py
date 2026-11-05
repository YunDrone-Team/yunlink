#!/usr/bin/env python3
"""The Sunray profile stays one package after the proto files are split."""

import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = ROOT / "profiles/com.yundrone.sunray/v2"
PARTS = (
    "sunray_state.proto",
    "sunray_uav.proto",
    "sunray_ugv.proto",
    "sunray_formation.proto",
    "sunray_services.proto",
)


class SunrayProtoSplitTest(unittest.TestCase):
    def test_entry_file_only_imports_the_parts(self):
        text = (PROFILE / "sunray.proto").read_text()
        self.assertNotRegex(text, r"^message |^enum ")
        for name in PARTS:
            self.assertIn(f'import public "com.yundrone.sunray/v2/{name}"', text)
            self.assertTrue((PROFILE / name).is_file())

    def test_names_and_packages_do_not_collide(self):
        names = []
        for path in PROFILE.glob("*.proto"):
            text = path.read_text()
            self.assertIn("package com.yundrone.sunray.v2;", text)
            names.extend(re.findall(r"^(?:message|enum) (\w+)", text, re.M))
        self.assertEqual(len(names), len(set(names)))
        self.assertIn("FlightControlState", names)
        self.assertIn("FcLogPullRequest", names)
        self.assertIn("FormationSetRequest", names)
        text = "\n".join((PROFILE / name).read_text() for name in PARTS)
        self.assertRegex(text, r"message FlightControlState \{[^}]*bool armed = 2;")
        self.assertRegex(text, r"message FcLogPullRequest \{[^}]*int32 id = 1;")
        self.assertRegex(text, r"message FormationSetRequest \{[^}]*string team_id = 11;")

    def test_descriptor_keeps_field_numbers(self):
        protoc = shutil.which("protoc")
        if not protoc:
            self.skipTest("protoc unavailable")
        try:
            from google.protobuf import descriptor_pb2, descriptor_pool, message_factory
        except ImportError:
            self.skipTest("python protobuf unavailable")
        with tempfile.TemporaryDirectory() as directory:
            descriptor = Path(directory) / "sunray.pb"
            subprocess.check_call(
                [
                    protoc,
                    "-I",
                    str(ROOT / "profiles"),
                    "--include_imports",
                    "--descriptor_set_out=" + str(descriptor),
                    str(PROFILE / "sunray.proto"),
                ]
            )
            descriptors = descriptor_pb2.FileDescriptorSet.FromString(descriptor.read_bytes())
        pool = descriptor_pool.DescriptorPool()
        for file in descriptors.file:
            pool.Add(file)
        factory = message_factory.MessageFactory(pool)
        create = getattr(message_factory, "GetMessageClass", None) or factory.GetPrototype
        state = create(pool.FindMessageTypeByName("com.yundrone.sunray.v2.FlightControlState"))
        pull = create(pool.FindMessageTypeByName("com.yundrone.sunray.v2.FcLogPullRequest"))
        formation = create(pool.FindMessageTypeByName("com.yundrone.sunray.v2.FormationSetRequest"))
        self.assertEqual(state.DESCRIPTOR.fields_by_name["armed"].number, 2)
        self.assertEqual(pull.DESCRIPTOR.fields_by_name["id"].number, 1)
        self.assertEqual(formation.DESCRIPTOR.fields_by_name["team_id"].number, 11)
        message = pull(id=28)
        self.assertEqual(create(pull.DESCRIPTOR).FromString(message.SerializeToString()).id, 28)


if __name__ == "__main__":
    unittest.main()

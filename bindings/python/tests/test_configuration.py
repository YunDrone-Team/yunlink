import math
import unittest

import yunlink
from yunlink.configuration_codec import ConfigurationCodecError, decode, encode


class ConfigurationCodecTest(unittest.TestCase):
    def test_patch_matches_cross_language_golden_vector(self) -> None:
        request = yunlink.ConfigResourcePatchRequest(
            resource_id="sunray.params.flight",
            expected_revision="rev-7",
            updates=(
                yunlink.ConfigFieldValue(
                    "control.max_speed", yunlink.ConfigValue.double(3.5)
                ),
                yunlink.ConfigFieldValue("control.enabled", yunlink.ConfigValue.bool(True)),
            ),
            variant_id="indoor",
            validate_only=True,
        )
        expected = bytes.fromhex(
            "140073756e7261792e706172616d732e666c696768740600696e646f6f720500"
            "7265762d3702001100636f6e74726f6c2e6d61785f737065656403000000000000"
            "0c400f00636f6e74726f6c2e656e61626c6564010101"
        )
        self.assertEqual(encode(request), expected)
        self.assertEqual(decode(yunlink.ConfigResourcePatchRequest, expected), request)

    def test_schema_and_variants_round_trip(self) -> None:
        resource = yunlink.ConfigResourceDescriptor(
            "sunray.params.flight", "Flight", "", True, True, True, True
        )
        response = yunlink.ConfigResourceDescribeResponse(
            yunlink.ConfigServiceStatus.OK,
            "ok",
            resource,
            (
                yunlink.ConfigFieldSchema(
                    "control.max_speed",
                    "Maximum speed",
                    "",
                    yunlink.ConfigValueType.DOUBLE,
                    minimum=0.0,
                    maximum=10.0,
                    group_path="control",
                    update_policy=yunlink.ConfigFieldUpdatePolicy.HOT_RELOAD,
                    unit="m/s",
                ),
            ),
        )
        self.assertEqual(decode(yunlink.ConfigResourceDescribeResponse, encode(response)), response)
        variants = yunlink.ConfigResourceVariantListResponse(
            yunlink.ConfigServiceStatus.OK,
            "ok",
            "indoor",
            (yunlink.ConfigVariantDescriptor("indoor", "Indoor", "r1", 42, True, True),),
        )
        self.assertEqual(
            decode(yunlink.ConfigResourceVariantListResponse, encode(variants)), variants
        )

        current = yunlink.ConfigSnapshot(
            "sunray.params.flight", "r1", "r1", "indoor", "indoor",
            (yunlink.ConfigFieldValue("control.max_speed", yunlink.ConfigValue.double(3.0)),),
        )
        candidate = yunlink.ConfigSnapshot(
            "sunray.params.flight", "candidate-2", "r1", "indoor", "indoor",
            (yunlink.ConfigFieldValue("control.max_speed", yunlink.ConfigValue.double(3.5)),),
        )
        preview = yunlink.ConfigResourcePatchResponse(
            yunlink.ConfigServiceStatus.OK, "validated", current, candidate_snapshot=candidate
        )
        self.assertEqual(
            encode(preview).hex(),
            "00090076616c696461746564140073756e7261792e706172616d732e666c696768740200"
            "7231020072310600696e646f6f720600696e646f6f7201001100636f6e74726f6c2e6d"
            "61785f737065656403000000000000084001140073756e7261792e706172616d732e666c"
            "696768740b0063616e6469646174652d32020072310600696e646f6f720600696e646f6f"
            "7201001100636f6e74726f6c2e6d61785f7370656564030000000000000c40000000000000",
        )
        self.assertEqual(
            decode(yunlink.ConfigResourcePatchResponse, encode(preview)), preview
        )

    def test_rejects_corruption_and_non_finite_values(self) -> None:
        request = yunlink.ConfigResourceGetRequest("sunray.params.flight", "")
        for malformed in (encode(request)[:-1], encode(request) + b"\0"):
            with self.assertRaises(ConfigurationCodecError):
                decode(yunlink.ConfigResourceGetRequest, malformed)
        invalid = yunlink.ConfigResourcePatchRequest(
            "sunray.params.flight",
            "r1",
            (yunlink.ConfigFieldValue("control.max_speed", yunlink.ConfigValue.double(math.nan)),),
        )
        with self.assertRaises(ConfigurationCodecError):
            encode(invalid)


if __name__ == "__main__":
    unittest.main()

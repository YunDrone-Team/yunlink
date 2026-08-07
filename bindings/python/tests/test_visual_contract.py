from pathlib import Path


VECTORS = (
    Path(__file__).resolve().parents[3]
    / "profiles/org.yunlink.visual/v1/golden/visual-v1-vectors.txt"
)


def _vectors() -> dict[str, str]:
    return {
        key: value
        for line in VECTORS.read_text(encoding="utf-8").splitlines()
        if line and not line.startswith("#")
        for key, value in [line.split("=", 1)]
    }


def _valid_ylpc(payload: bytes) -> bool:
    if len(payload) < 16 or payload[:4] != b"YLPC":
        return False
    version = int.from_bytes(payload[4:6], "little")
    flags = int.from_bytes(payload[6:8], "little")
    count = int.from_bytes(payload[8:12], "little")
    stride = int.from_bytes(payload[12:16], "little")
    return version == 1 and flags & ~1 == 0 and stride == 16 and len(payload) == 16 + count * 16


def test_shared_visual_vectors_define_the_v1_contract():
    vectors = _vectors()
    assert _valid_ylpc(bytes.fromhex(vectors["point_cloud.valid.hex"]))
    assert not _valid_ylpc(bytes.fromhex(vectors["point_cloud.invalid_stride.hex"]))
    assert not _valid_ylpc(bytes.fromhex(vectors["point_cloud.invalid_truncated.hex"]))
    assert bytes.fromhex(vectors["image.raw.hex"]) == bytes(range(6))
    assert '"lifetime_ns"' in vectors["marker.add.json"]
    assert '"action":2' in vectors["marker.delete.json"]
    assert "[0,0,0,0]" in vectors["marker.invalid_quaternion.json"]

#include "com.yundrone.sunray/v2/control_validation.hpp"

#include <cassert>
#include <string>
#include <vector>

namespace sunray = com::yundrone::sunray::v2;

int main() {
    sunray::FormationSetRequest baseline;
    baseline.set_formation_type(sunray::FORMATION_TAKEOFF);
    std::string error;
    assert(sunray::validate_formation_set_request(baseline, &error));
    const std::string wire = baseline.SerializeAsString();
    // Literal pre-removal protobuf encodings: explicit height enum (8),
    // absolute height including zero (9), double-ring payload including empty (10).
    const std::vector<std::string> removed = {
        std::string("\x40\x01", 2), std::string("\x49\0\0\0\0\0\0\0\0", 9),
        std::string("\x52\0", 2), std::string("\x40\0", 2)};
    for (const auto& suffix : removed) {
        sunray::FormationSetRequest decoded;
        assert(decoded.ParseFromString(wire + suffix));
        error.clear();
        assert(!sunray::validate_formation_set_request(decoded, &error));
        assert(error.find("removed formation") != std::string::npos);
        // The preview RPC must share the same fail-closed validation.
        sunray::FormationPreviewRequest preview;
        preview.mutable_reference()->mutable_position();
        preview.mutable_reference()->mutable_orientation()->set_w(1.0);
        *preview.mutable_goal() = baseline;
        assert(sunray::validate_formation_preview_request(preview, &error));
        *preview.mutable_goal() = decoded;
        assert(!sunray::validate_formation_preview_request(preview, &error));
    }
    // Reserving the retired semantics does not reject every future extension.
    sunray::FormationSetRequest future;
    assert(future.ParseFromString(wire + std::string("\x90\x03\x01", 3)));
    assert(sunray::validate_formation_set_request(future, &error));
    return 0;
}

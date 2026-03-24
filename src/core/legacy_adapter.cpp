/**
 * @file src/core/legacy_adapter.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/compat/legacy_adapter.hpp"

namespace sunraycom::compat {

ByteBuffer LegacyAdapter::encode_legacy(const ::DataFrame& frame) const {
    return legacy_codec_.coder(frame);
}

bool LegacyAdapter::decode_legacy(const ByteBuffer& bytes, ::DataFrame* out) const {
    if (out == nullptr)
        return false;
    return legacy_codec_.decoder(bytes, *out);
}

std::optional<Frame> LegacyAdapter::to_core_frame(const ::DataFrame& legacy_frame) const {
    const ByteBuffer bytes = legacy_codec_.coder(legacy_frame);
    if (bytes.empty())
        return std::nullopt;
    const DecodeResult dr = core_codec_.decode(bytes.data(), bytes.size());
    if (!dr.ok())
        return std::nullopt;
    return dr.frame;
}

bool LegacyAdapter::from_core_frame(const Frame& frame, ::DataFrame* out) const {
    if (out == nullptr)
        return false;
    const ByteBuffer bytes = core_codec_.encode(frame, false);
    if (bytes.empty())
        return false;
    return legacy_codec_.decoder(bytes, *out);
}

}  // namespace sunraycom::compat

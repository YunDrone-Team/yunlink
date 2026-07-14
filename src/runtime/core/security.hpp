/**
 * @file src/runtime/core/security.hpp
 * @brief Runtime envelope validation and security helpers.
 */

#ifndef YUNLINK_RUNTIME_CORE_SECURITY_HPP
#define YUNLINK_RUNTIME_CORE_SECURITY_HPP

#include "internal.hpp"

namespace yunlink {

bool runtime_envelope_expired(const SecureEnvelope& envelope, uint64_t now_ms);
bool runtime_protocol_version_mismatch(const SecureEnvelope& envelope);
bool runtime_schema_version_mismatch(const SecureEnvelope& envelope);
bool runtime_security_tags_required(const RuntimeConfig& config);
bool runtime_security_tags_enabled(const RuntimeConfig& config);
ByteBuffer make_runtime_auth_tag(const RuntimeConfig& config, const SecureEnvelope& envelope);
void apply_runtime_security_tag(const RuntimeConfig& config, SecureEnvelope* envelope);
std::string runtime_security_replay_key(const SecureEnvelope& envelope);

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_CORE_SECURITY_HPP

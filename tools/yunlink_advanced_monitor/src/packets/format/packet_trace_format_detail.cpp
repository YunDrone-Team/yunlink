#include "packets/format/packet_trace_format.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace {

std::string agent_label(yunlink::AgentType type) {
    switch (type) {
    case yunlink::AgentType::kGroundStation:
        return "GroundStation";
    case yunlink::AgentType::kUav:
        return "UAV";
    case yunlink::AgentType::kUgv:
        return "UGV";
    case yunlink::AgentType::kSwarmController:
        return "SwarmController";
    case yunlink::AgentType::kUnknown:
        return "Unknown";
    }
    return "Unknown";
}

std::string role_label(yunlink::EndpointRole role) {
    switch (role) {
    case yunlink::EndpointRole::kObserver:
        return "Observer";
    case yunlink::EndpointRole::kController:
        return "Controller";
    case yunlink::EndpointRole::kVehicle:
        return "Vehicle";
    case yunlink::EndpointRole::kRelay:
        return "Relay";
    case yunlink::EndpointRole::kUnknown:
        return "Unknown";
    }
    return "Unknown";
}

}  // namespace

std::string packet_endpoint_label(const yunlink::EndpointIdentity& identity) {
    std::ostringstream ss;
    ss << agent_label(identity.agent_type) << "/" << identity.agent_id << " role="
       << role_label(identity.role);
    return ss.str();
}

std::string packet_target_label(const yunlink::TargetSelector& target) {
    std::ostringstream ss;
    ss << "scope=";
    switch (target.scope) {
    case yunlink::TargetScope::kEntity:
        ss << "Entity";
        break;
    case yunlink::TargetScope::kGroup:
        ss << "Group";
        break;
    case yunlink::TargetScope::kBroadcast:
        ss << "Broadcast";
        break;
    }
    ss << " type=" << agent_label(target.target_type) << " group=" << target.group_id << " ids=";
    if (target.target_ids.empty()) {
        ss << "-";
    } else {
        for (size_t i = 0; i < target.target_ids.size(); ++i) {
            if (i > 0) {
                ss << ",";
            }
            ss << target.target_ids[i];
        }
    }
    return ss.str();
}

std::string packet_hex_dump(const yunlink::ByteBuffer& bytes) {
    std::ostringstream ss;
    for (size_t offset = 0; offset < bytes.size(); offset += 16) {
        ss << std::setw(6) << std::setfill('0') << std::hex << offset << "  ";
        for (size_t i = 0; i < 16; ++i) {
            if (offset + i < bytes.size()) {
                ss << std::setw(2) << static_cast<int>(bytes[offset + i]) << ' ';
            } else {
                ss << "   ";
            }
        }
        ss << " ";
        for (size_t i = 0; i < 16 && offset + i < bytes.size(); ++i) {
            const unsigned char ch = bytes[offset + i];
            ss << (std::isprint(ch) ? static_cast<char>(ch) : '.');
        }
        ss << '\n';
    }
    return ss.str();
}

std::string packet_ascii_preview(const yunlink::ByteBuffer& bytes) {
    std::string out;
    out.reserve(bytes.size());
    for (uint8_t byte : bytes) {
        const unsigned char ch = byte;
        out.push_back(std::isprint(ch) || ch == '\n' || ch == '\r' || ch == '\t'
                          ? static_cast<char>(ch)
                          : '.');
    }
    return out;
}

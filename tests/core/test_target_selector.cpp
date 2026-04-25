/**
 * @file tests/core/test_target_selector.cpp
 * @brief TargetSelector matching semantics tests.
 */

#include <iostream>

#include "yunlink/core/types.hpp"

int main() {
    yunlink::EndpointIdentity uav{};
    uav.agent_type = yunlink::AgentType::kUav;
    uav.agent_id = 9;
    uav.role = yunlink::EndpointRole::kVehicle;
    uav.group_ids = {42};

    if (!yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9).matches(uav)) {
        std::cerr << "entity target did not match own id\n";
        return 1;
    }
    if (!yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 42).matches(uav)) {
        std::cerr << "group target did not match joined group\n";
        return 2;
    }
    if (yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 7).matches(uav)) {
        std::cerr << "group target matched wrong group id\n";
        return 3;
    }
    if (yunlink::TargetSelector::for_group(yunlink::AgentType::kUgv, 42).matches(uav)) {
        std::cerr << "group target matched wrong agent type\n";
        return 4;
    }

    yunlink::EndpointIdentity ungrouped = uav;
    ungrouped.group_ids.clear();
    if (yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 42).matches(ungrouped)) {
        std::cerr << "group target matched endpoint without group membership\n";
        return 5;
    }

    if (!yunlink::TargetSelector::broadcast(yunlink::AgentType::kUav).matches(ungrouped)) {
        std::cerr << "broadcast target stopped matching agent type\n";
        return 6;
    }

    return 0;
}

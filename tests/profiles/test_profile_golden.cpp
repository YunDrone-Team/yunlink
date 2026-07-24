#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

#include "com.yundrone.sunray/v1/sunray.pb.h"
#include "org.yunlink.mobility/v1/mobility.pb.h"

namespace {

std::string hex(const std::string& value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char byte : value) {
        stream << std::setw(2) << static_cast<unsigned>(byte);
    }
    return stream.str();
}

}  // namespace

int main() {
    org::yunlink::mobility::v1::GotoGoal goal;
    goal.set_frame_id("map");
    goal.mutable_position()->set_x(1.0);
    goal.mutable_position()->set_y(-2.0);
    goal.mutable_position()->set_z(0.5);
    goal.set_yaw_rad(0.25);
    assert(hex(goal.SerializeAsString()) ==
           "0a036d6170121b09000000000000f03f1100000000000000c019000000000000e03f190000000000"
           "00d03f");

    com::yundrone::sunray::v1::FeatureStartRequest request;
    request.set_name("mapping");
    assert(hex(request.SerializeAsString()) == "0a076d617070696e67");

    const std::string damaged("\x0a\x08mapping", 9);
    assert(!request.ParseFromString(damaged));
    return 0;
}

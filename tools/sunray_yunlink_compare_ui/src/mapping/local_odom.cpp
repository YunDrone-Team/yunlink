#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_local_odom_from_ros(const nav_msgs::Odometry& msg,
                              std::unordered_map<std::string, std::string>& values) {
    set_odometry(values, "", msg);
}

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_odometry(values, "", msg);
}

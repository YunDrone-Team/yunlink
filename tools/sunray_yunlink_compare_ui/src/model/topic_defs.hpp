#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_DEFS_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_DEFS_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "model/topic_state.hpp"

std::unordered_map<std::string, TopicState> make_default_topics();
std::vector<std::string> topic_display_order();

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_DEFS_HPP

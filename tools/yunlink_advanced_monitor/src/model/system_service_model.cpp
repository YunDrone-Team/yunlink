#include "model/system_service_model.hpp"

std::string system_service_lifecycle_label(MonitorSystemServiceLifecycle lifecycle) {
    switch (lifecycle) {
    case MonitorSystemServiceLifecycle::kPending:
        return "PENDING";
    case MonitorSystemServiceLifecycle::kSucceeded:
        return "SUCCEEDED";
    case MonitorSystemServiceLifecycle::kFailed:
        return "FAILED";
    case MonitorSystemServiceLifecycle::kTimeout:
        return "TIMEOUT";
    }
    return "PENDING";
}

#include "org.yunlink.shell/v1/shell_validation.hpp"

namespace org::yunlink::shell::v1 {
namespace {
bool fail(const char* message, std::string* error) {
    if (error != nullptr) *error = message;
    return false;
}
bool valid_window(uint32_t cols, uint32_t rows) {
    return cols >= 20U && cols <= 400U && rows >= 5U && rows <= 200U;
}
}  // namespace

bool validate_open_request(const ShellOpenRequest& request, std::string* error) {
    return valid_window(request.cols(), request.rows()) || fail("invalid shell window", error);
}
bool validate_write_request(const ShellWriteRequest& request, std::string* error) {
    return !request.session_uid().empty() && request.data().size() <= 16U * 1024U
        ? true : fail("invalid shell write request", error);
}
bool validate_resize_request(const ShellResizeRequest& request, std::string* error) {
    return !request.session_uid().empty() && valid_window(request.cols(), request.rows())
        ? true : fail("invalid shell resize request", error);
}
bool validate_close_request(const ShellCloseRequest& request, std::string* error) {
    return !request.session_uid().empty() || fail("shell session UID is required", error);
}
}  // namespace org::yunlink::shell::v1

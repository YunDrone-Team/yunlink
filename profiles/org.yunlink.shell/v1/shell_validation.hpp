#pragma once

#include <string>

#include "org.yunlink.shell/v1/shell.pb.h"

namespace org::yunlink::shell::v1 {

bool validate_open_request(const ShellOpenRequest& request, std::string* error = nullptr);
bool validate_write_request(const ShellWriteRequest& request, std::string* error = nullptr);
bool validate_resize_request(const ShellResizeRequest& request, std::string* error = nullptr);
bool validate_close_request(const ShellCloseRequest& request, std::string* error = nullptr);

}  // namespace org::yunlink::shell::v1

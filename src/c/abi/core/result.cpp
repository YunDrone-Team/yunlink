/**
 * @file src/c/abi/result.cpp
 * @brief C ABI result string conversion.
 */

#include "../internal.hpp"

extern "C" {

const char* yunlink_result_name(yunlink_result_t result) {
    switch (result) {
    case YUNLINK_RESULT_OK:
        return "YUNLINK_RESULT_OK";
    case YUNLINK_RESULT_INVALID_ARGUMENT:
        return "YUNLINK_RESULT_INVALID_ARGUMENT";
    case YUNLINK_RESULT_SOCKET_ERROR:
        return "YUNLINK_RESULT_SOCKET_ERROR";
    case YUNLINK_RESULT_BIND_ERROR:
        return "YUNLINK_RESULT_BIND_ERROR";
    case YUNLINK_RESULT_LISTEN_ERROR:
        return "YUNLINK_RESULT_LISTEN_ERROR";
    case YUNLINK_RESULT_CONNECT_ERROR:
        return "YUNLINK_RESULT_CONNECT_ERROR";
    case YUNLINK_RESULT_TIMEOUT:
        return "YUNLINK_RESULT_TIMEOUT";
    case YUNLINK_RESULT_ENCODE_ERROR:
        return "YUNLINK_RESULT_ENCODE_ERROR";
    case YUNLINK_RESULT_DECODE_ERROR:
        return "YUNLINK_RESULT_DECODE_ERROR";
    case YUNLINK_RESULT_CHECKSUM_MISMATCH:
        return "YUNLINK_RESULT_CHECKSUM_MISMATCH";
    case YUNLINK_RESULT_INVALID_HEADER:
        return "YUNLINK_RESULT_INVALID_HEADER";
    case YUNLINK_RESULT_RUNTIME_STOPPED:
        return "YUNLINK_RESULT_RUNTIME_STOPPED";
    case YUNLINK_RESULT_PROTOCOL_MISMATCH:
        return "YUNLINK_RESULT_PROTOCOL_MISMATCH";
    case YUNLINK_RESULT_UNAUTHORIZED:
        return "YUNLINK_RESULT_UNAUTHORIZED";
    case YUNLINK_RESULT_REJECTED:
        return "YUNLINK_RESULT_REJECTED";
    case YUNLINK_RESULT_INTERNAL:
        return "YUNLINK_RESULT_INTERNAL";
    case YUNLINK_RESULT_NOT_FOUND:
        return "YUNLINK_RESULT_NOT_FOUND";
    }
    return "YUNLINK_RESULT_UNKNOWN";
}

}  // extern "C"

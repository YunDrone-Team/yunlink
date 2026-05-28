/**
 * @file include/yunlink/c/abi/base.h
 * @brief C ABI base macros and runtime handle declarations.
 */

#ifndef YUNLINK_C_ABI_BASE_H
#define YUNLINK_C_ABI_BASE_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#ifdef YUNLINK_FFI_SHARED_BUILD
#define YUNLINK_C_API __declspec(dllexport)
#elif defined(YUNLINK_FFI_SHARED_USE)
#define YUNLINK_C_API __declspec(dllimport)
#else
#define YUNLINK_C_API
#endif
#else
#define YUNLINK_C_API __attribute__((visibility("default")))
#endif

#define YUNLINK_FFI_ABI_VERSION 1u

typedef struct yunlink_runtime yunlink_runtime_t;

#endif  // YUNLINK_C_ABI_BASE_H

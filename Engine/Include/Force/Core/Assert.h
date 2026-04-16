#pragma once

#include "Force/Core/Logger.h"
#include <filesystem>

#ifdef FORCE_ENABLE_ASSERTS
    #ifdef FORCE_PLATFORM_WINDOWS
        #define FORCE_DEBUGBREAK() __debugbreak()
    #else
        #include <signal.h>
        #define FORCE_DEBUGBREAK() raise(SIGTRAP)
    #endif

    #define FORCE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) \
        do { \
            if (!(check)) { \
                FORCE##type##ERROR("Assertion Failed: {}", msg); \
                FORCE##type##ERROR("  File: {}", __FILE__); \
                FORCE##type##ERROR("  Line: {}", __LINE__); \
                FORCE_DEBUGBREAK(); \
            } \
        } while (false)

    #define FORCE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) FORCE_INTERNAL_ASSERT_IMPL(type, check, __VA_ARGS__)
    #define FORCE_INTERNAL_ASSERT_NO_MSG(type, check) FORCE_INTERNAL_ASSERT_IMPL(type, check, "No message provided")

    #define FORCE_ASSERT(check, ...) FORCE_INTERNAL_ASSERT_WITH_MSG(_, check, __VA_ARGS__)
    #define FORCE_CORE_ASSERT(check, ...) FORCE_INTERNAL_ASSERT_WITH_MSG(_CORE_, check, __VA_ARGS__)
    
    #define FORCE_VERIFY(check, ...) FORCE_ASSERT(check, __VA_ARGS__)
    #define FORCE_CORE_VERIFY(check, ...) FORCE_CORE_ASSERT(check, __VA_ARGS__)
#else
    #define FORCE_DEBUGBREAK()
    #define FORCE_ASSERT(check, ...)
    #define FORCE_CORE_ASSERT(check, ...)
    #define FORCE_VERIFY(check, ...) (void)(check)
    #define FORCE_CORE_VERIFY(check, ...) (void)(check)
#endif

// Static assert wrapper
#define FORCE_STATIC_ASSERT(condition, msg) static_assert(condition, msg)

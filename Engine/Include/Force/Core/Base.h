#pragma once

// Force Engine Base Header - Precompiled Header

// Platform detection
#ifdef _WIN32
    #define FORCE_PLATFORM_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#elif defined(__linux__)
    #define FORCE_PLATFORM_LINUX
#elif defined(__APPLE__)
    #define FORCE_PLATFORM_MACOS
#endif

// Build configuration
#ifdef FORCE_DEBUG
    #define FORCE_ENABLE_ASSERTS
#endif

// DLL export/import
#ifdef FORCE_PLATFORM_WINDOWS
    #ifdef FORCE_BUILD_DLL
        #define FORCE_API __declspec(dllexport)
    #else
        #define FORCE_API
    #endif
#else
    #define FORCE_API
#endif

// Standard library includes
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <variant>
#include <span>
#include <filesystem>
#include <format>

// Smart pointer aliases
namespace Force
{
    template<typename T>
    using Scope = std::unique_ptr<T>;
    
    template<typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    using Ref = std::shared_ptr<T>;
    
    template<typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    using WeakRef = std::weak_ptr<T>;
}

// Core includes
#include "Force/Core/Types.h"
#include "Force/Core/Logger.h"
#include "Force/Core/Assert.h"

// Profiling
#ifdef FORCE_PROFILING_ENABLED
    #include <tracy/Tracy.hpp>
    #define FORCE_PROFILE_SCOPE(name) ZoneScopedN(name)
    #define FORCE_PROFILE_FUNCTION() ZoneScoped
    #define FORCE_PROFILE_FRAME_MARK FrameMark
#else
    #define FORCE_PROFILE_SCOPE(name)
    #define FORCE_PROFILE_FUNCTION()
    #define FORCE_PROFILE_FRAME_MARK
#endif

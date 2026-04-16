#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <memory>
#include <string_view>

namespace Force
{
    class Logger
    {
    public:
        static void Init();
        static void Shutdown();
        
        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
        
    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// Core logging macros
#define FORCE_CORE_TRACE(...)    ::Force::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define FORCE_CORE_DEBUG(...)    ::Force::Logger::GetCoreLogger()->debug(__VA_ARGS__)
#define FORCE_CORE_INFO(...)     ::Force::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define FORCE_CORE_WARN(...)     ::Force::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define FORCE_CORE_ERROR(...)    ::Force::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define FORCE_CORE_CRITICAL(...) ::Force::Logger::GetCoreLogger()->critical(__VA_ARGS__)

// Client logging macros
#define FORCE_TRACE(...)    ::Force::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define FORCE_DEBUG(...)    ::Force::Logger::GetClientLogger()->debug(__VA_ARGS__)
#define FORCE_INFO(...)     ::Force::Logger::GetClientLogger()->info(__VA_ARGS__)
#define FORCE_WARN(...)     ::Force::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define FORCE_ERROR(...)    ::Force::Logger::GetClientLogger()->error(__VA_ARGS__)
#define FORCE_CRITICAL(...) ::Force::Logger::GetClientLogger()->critical(__VA_ARGS__)

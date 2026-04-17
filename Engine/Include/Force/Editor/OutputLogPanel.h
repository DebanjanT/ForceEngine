#pragma once
#include "Force/Core/Base.h"
#include <imgui.h>
#include <spdlog/sinks/base_sink.h>
#include <string>
#include <vector>
#include <mutex>

namespace Force
{
    enum class LogLevel { Trace, Info, Warn, Error, Critical };

    struct LogEntry
    {
        LogLevel    Level;
        std::string Message;
        std::string Logger;
    };

    // spdlog sink that stores entries
    class ImGuiLogSink : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        static ImGuiLogSink& Get();

        const std::vector<LogEntry>& GetEntries() const { return m_Entries; }
        void Clear() { std::lock_guard<std::mutex> lk(mutex_); m_Entries.clear(); }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override;
        void flush_() override {}

    private:
        std::vector<LogEntry> m_Entries;
        static constexpr size_t k_MaxEntries = 2000;
    };

    class OutputLogPanel
    {
    public:
        void OnImGui();
        bool m_Open = true;

    private:
        bool m_AutoScroll   = true;
        bool m_ShowTrace    = true;
        bool m_ShowInfo     = true;
        bool m_ShowWarn     = true;
        bool m_ShowError    = true;
        bool m_ShowCritical = true;
        char m_FilterBuf[128] = {};
    };
}

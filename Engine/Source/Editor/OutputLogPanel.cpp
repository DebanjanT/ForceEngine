#include "Force/Editor/OutputLogPanel.h"
#include <imgui_internal.h>
#include <spdlog/details/log_msg.h>
#include <algorithm>

namespace Force
{
    ImGuiLogSink& ImGuiLogSink::Get()
    {
        static ImGuiLogSink instance;
        return instance;
    }

    void ImGuiLogSink::sink_it_(const spdlog::details::log_msg& msg)
    {
        LogEntry entry;
        entry.Logger  = std::string(msg.logger_name.begin(), msg.logger_name.end());
        entry.Message = std::string(msg.payload.begin(), msg.payload.end());

        switch (msg.level)
        {
            case spdlog::level::trace:    entry.Level = LogLevel::Trace;    break;
            case spdlog::level::info:     entry.Level = LogLevel::Info;     break;
            case spdlog::level::warn:     entry.Level = LogLevel::Warn;     break;
            case spdlog::level::err:      entry.Level = LogLevel::Error;    break;
            case spdlog::level::critical: entry.Level = LogLevel::Critical; break;
            default:                      entry.Level = LogLevel::Trace;    break;
        }

        if (m_Entries.size() >= k_MaxEntries)
            m_Entries.erase(m_Entries.begin(), m_Entries.begin() + 200);
        m_Entries.push_back(std::move(entry));
    }

    static ImVec4 LevelColor(LogLevel lv)
    {
        switch (lv)
        {
            case LogLevel::Trace:    return ImVec4(0.6f,0.6f,0.6f,1.f);
            case LogLevel::Info:     return ImVec4(0.8f,0.9f,0.8f,1.f);
            case LogLevel::Warn:     return ImVec4(0.95f,0.85f,0.2f,1.f);
            case LogLevel::Error:    return ImVec4(1.f,0.35f,0.35f,1.f);
            case LogLevel::Critical: return ImVec4(1.f,0.0f,0.0f,1.f);
        }
        return ImVec4(1,1,1,1);
    }

    static const char* LevelStr(LogLevel lv)
    {
        switch (lv)
        {
            case LogLevel::Trace:    return "[TRACE]";
            case LogLevel::Info:     return "[INFO] ";
            case LogLevel::Warn:     return "[WARN] ";
            case LogLevel::Error:    return "[ERROR]";
            case LogLevel::Critical: return "[CRIT] ";
        }
        return "";
    }

    void OutputLogPanel::OnImGui()
    {
        if (!m_Open) return;
        ImGui::Begin("Output Log", &m_Open);

        // Toolbar
        if (ImGui::SmallButton("Clear")) ImGuiLogSink::Get().Clear();
        ImGui::SameLine();
        ImGui::Checkbox("T", &m_ShowTrace);   ImGui::SameLine();
        ImGui::Checkbox("I", &m_ShowInfo);    ImGui::SameLine();
        ImGui::Checkbox("W", &m_ShowWarn);    ImGui::SameLine();
        ImGui::Checkbox("E", &m_ShowError);   ImGui::SameLine();
        ImGui::Checkbox("C", &m_ShowCritical);ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        ImGui::InputText("##filter", m_FilterBuf, sizeof(m_FilterBuf));
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
        ImGui::Separator();

        ImGui::BeginChild("##log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        std::string filter(m_FilterBuf);
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

        auto& entries = ImGuiLogSink::Get().GetEntries();
        ImGuiListClipper clipper;
        clipper.Begin((int)entries.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const auto& e = entries[i];

                // Level filter
                if (!m_ShowTrace    && e.Level == LogLevel::Trace)    continue;
                if (!m_ShowInfo     && e.Level == LogLevel::Info)     continue;
                if (!m_ShowWarn     && e.Level == LogLevel::Warn)     continue;
                if (!m_ShowError    && e.Level == LogLevel::Error)    continue;
                if (!m_ShowCritical && e.Level == LogLevel::Critical) continue;

                // Text filter
                if (!filter.empty())
                {
                    std::string msg = e.Message;
                    std::transform(msg.begin(), msg.end(), msg.begin(), ::tolower);
                    if (msg.find(filter) == std::string::npos) continue;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(e.Level));
                ImGui::TextUnformatted(LevelStr(e.Level));
                ImGui::SameLine();
                ImGui::TextUnformatted(e.Message.c_str());
                ImGui::PopStyleColor();
            }
        }
        clipper.End();

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.f);

        ImGui::EndChild();
        ImGui::End();
    }
}

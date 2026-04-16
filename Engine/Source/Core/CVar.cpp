#include "Force/Core/CVar.h"
#include "Force/Core/Logger.h"
#include "Force/Platform/Platform.h"
#include <sstream>
#include <fstream>
#include <algorithm>

namespace Force
{
    // CVar Implementation
    CVar::CVar(const std::string& name, bool defaultValue, const std::string& description, CVarFlags flags)
        : m_Name(name), m_Description(description), m_Flags(flags), m_Value(defaultValue), m_DefaultValue(defaultValue)
    {
    }
    
    CVar::CVar(const std::string& name, i32 defaultValue, const std::string& description, CVarFlags flags)
        : m_Name(name), m_Description(description), m_Flags(flags), m_Value(defaultValue), m_DefaultValue(defaultValue)
    {
    }
    
    CVar::CVar(const std::string& name, f32 defaultValue, const std::string& description, CVarFlags flags)
        : m_Name(name), m_Description(description), m_Flags(flags), m_Value(defaultValue), m_DefaultValue(defaultValue)
    {
    }
    
    CVar::CVar(const std::string& name, const std::string& defaultValue, const std::string& description, CVarFlags flags)
        : m_Name(name), m_Description(description), m_Flags(flags), m_Value(defaultValue), m_DefaultValue(defaultValue)
    {
    }
    
    bool CVar::GetBool() const
    {
        return std::get<bool>(m_Value);
    }
    
    i32 CVar::GetInt() const
    {
        return std::get<i32>(m_Value);
    }
    
    f32 CVar::GetFloat() const
    {
        return std::get<f32>(m_Value);
    }
    
    const std::string& CVar::GetString() const
    {
        return std::get<std::string>(m_Value);
    }
    
    void CVar::SetBool(bool value)
    {
        if (HasFlag(m_Flags, CVarFlags::ReadOnly))
        {
            FORCE_CORE_WARN("CVar {} is read-only", m_Name);
            return;
        }
        m_Value = value;
        NotifyChange();
    }
    
    void CVar::SetInt(i32 value)
    {
        if (HasFlag(m_Flags, CVarFlags::ReadOnly))
        {
            FORCE_CORE_WARN("CVar {} is read-only", m_Name);
            return;
        }
        m_Value = value;
        NotifyChange();
    }
    
    void CVar::SetFloat(f32 value)
    {
        if (HasFlag(m_Flags, CVarFlags::ReadOnly))
        {
            FORCE_CORE_WARN("CVar {} is read-only", m_Name);
            return;
        }
        m_Value = value;
        NotifyChange();
    }
    
    void CVar::SetString(const std::string& value)
    {
        if (HasFlag(m_Flags, CVarFlags::ReadOnly))
        {
            FORCE_CORE_WARN("CVar {} is read-only", m_Name);
            return;
        }
        m_Value = value;
        NotifyChange();
    }
    
    void CVar::SetFromString(const std::string& value)
    {
        if (std::holds_alternative<bool>(m_Value))
        {
            SetBool(value == "true" || value == "1");
        }
        else if (std::holds_alternative<i32>(m_Value))
        {
            SetInt(std::stoi(value));
        }
        else if (std::holds_alternative<f32>(m_Value))
        {
            SetFloat(std::stof(value));
        }
        else if (std::holds_alternative<std::string>(m_Value))
        {
            SetString(value);
        }
    }
    
    std::string CVar::GetAsString() const
    {
        if (std::holds_alternative<bool>(m_Value))
        {
            return std::get<bool>(m_Value) ? "true" : "false";
        }
        else if (std::holds_alternative<i32>(m_Value))
        {
            return std::to_string(std::get<i32>(m_Value));
        }
        else if (std::holds_alternative<f32>(m_Value))
        {
            return std::to_string(std::get<f32>(m_Value));
        }
        else if (std::holds_alternative<std::string>(m_Value))
        {
            return std::get<std::string>(m_Value);
        }
        return "";
    }
    
    void CVar::Reset()
    {
        m_Value = m_DefaultValue;
        NotifyChange();
    }
    
    void CVar::AddChangeCallback(ChangeCallback callback)
    {
        m_ChangeCallbacks.push_back(callback);
    }
    
    void CVar::NotifyChange()
    {
        for (auto& callback : m_ChangeCallbacks)
        {
            callback(*this);
        }
    }
    
    // CVarSystem Implementation
    CVarSystem& CVarSystem::Get()
    {
        static CVarSystem instance;
        return instance;
    }
    
    CVar* CVarSystem::Register(const std::string& name, bool defaultValue, const std::string& description, CVarFlags flags)
    {
        if (Exists(name))
        {
            FORCE_CORE_WARN("CVar {} already exists", name);
            return Find(name);
        }
        m_CVars[name] = CreateScope<CVar>(name, defaultValue, description, flags);
        return m_CVars[name].get();
    }
    
    CVar* CVarSystem::Register(const std::string& name, i32 defaultValue, const std::string& description, CVarFlags flags)
    {
        if (Exists(name))
        {
            FORCE_CORE_WARN("CVar {} already exists", name);
            return Find(name);
        }
        m_CVars[name] = CreateScope<CVar>(name, defaultValue, description, flags);
        return m_CVars[name].get();
    }
    
    CVar* CVarSystem::Register(const std::string& name, f32 defaultValue, const std::string& description, CVarFlags flags)
    {
        if (Exists(name))
        {
            FORCE_CORE_WARN("CVar {} already exists", name);
            return Find(name);
        }
        m_CVars[name] = CreateScope<CVar>(name, defaultValue, description, flags);
        return m_CVars[name].get();
    }
    
    CVar* CVarSystem::Register(const std::string& name, const std::string& defaultValue, const std::string& description, CVarFlags flags)
    {
        if (Exists(name))
        {
            FORCE_CORE_WARN("CVar {} already exists", name);
            return Find(name);
        }
        m_CVars[name] = CreateScope<CVar>(name, defaultValue, description, flags);
        return m_CVars[name].get();
    }
    
    CVar* CVarSystem::Find(const std::string& name)
    {
        auto it = m_CVars.find(name);
        return it != m_CVars.end() ? it->second.get() : nullptr;
    }
    
    const CVar* CVarSystem::Find(const std::string& name) const
    {
        auto it = m_CVars.find(name);
        return it != m_CVars.end() ? it->second.get() : nullptr;
    }
    
    bool CVarSystem::Exists(const std::string& name) const
    {
        return m_CVars.find(name) != m_CVars.end();
    }
    
    void CVarSystem::SetFromString(const std::string& name, const std::string& value)
    {
        CVar* cvar = Find(name);
        if (cvar)
        {
            cvar->SetFromString(value);
        }
        else
        {
            FORCE_CORE_WARN("CVar {} not found", name);
        }
    }
    
    std::string CVarSystem::GetAsString(const std::string& name) const
    {
        const CVar* cvar = Find(name);
        return cvar ? cvar->GetAsString() : "";
    }
    
    void CVarSystem::LoadFromFile(const std::string& filepath)
    {
        std::string content = Platform::ReadFileText(filepath);
        if (content.empty()) return;
        
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line))
        {
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string name = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                SetFromString(name, value);
            }
        }
        
        FORCE_CORE_INFO("Loaded CVars from: {}", filepath);
    }
    
    void CVarSystem::SaveToFile(const std::string& filepath) const
    {
        std::ostringstream stream;
        stream << "# Force Engine Configuration\n\n";
        
        for (const auto& [name, cvar] : m_CVars)
        {
            if (HasFlag(cvar->GetFlags(), CVarFlags::Archive))
            {
                stream << name << " = " << cvar->GetAsString() << "\n";
            }
        }
        
        Platform::WriteFile(filepath, stream.str().data(), stream.str().size());
        FORCE_CORE_INFO("Saved CVars to: {}", filepath);
    }
    
    void CVarSystem::ExecuteCommand(const std::string& command)
    {
        std::istringstream stream(command);
        std::string name;
        stream >> name;
        
        CVar* cvar = Find(name);
        if (!cvar)
        {
            FORCE_CORE_WARN("Unknown command/cvar: {}", name);
            return;
        }
        
        std::string value;
        if (stream >> value)
        {
            cvar->SetFromString(value);
            FORCE_CORE_INFO("{} = {}", name, cvar->GetAsString());
        }
        else
        {
            FORCE_CORE_INFO("{} = {} ({})", name, cvar->GetAsString(), cvar->GetDescription());
        }
    }
    
    std::vector<std::string> CVarSystem::GetAllCVarNames() const
    {
        std::vector<std::string> names;
        for (const auto& [name, cvar] : m_CVars)
        {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }
    
    std::vector<std::string> CVarSystem::AutoComplete(const std::string& prefix) const
    {
        std::vector<std::string> matches;
        for (const auto& [name, cvar] : m_CVars)
        {
            if (name.find(prefix) == 0)
            {
                matches.push_back(name);
            }
        }
        std::sort(matches.begin(), matches.end());
        return matches;
    }
}

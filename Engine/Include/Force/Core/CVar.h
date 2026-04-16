#pragma once

#include "Force/Core/Base.h"
#include <unordered_map>
#include <variant>
#include <functional>

namespace Force
{
    enum class CVarFlags : u32
    {
        None        = 0,
        ReadOnly    = 1 << 0,  // Cannot be changed at runtime
        Cheat       = 1 << 1,  // Only available in dev builds
        Archive     = 1 << 2,  // Saved to config file
        RequiresRestart = 1 << 3,  // Changes require restart
        Hidden      = 1 << 4   // Not shown in console
    };
    
    inline CVarFlags operator|(CVarFlags a, CVarFlags b)
    {
        return static_cast<CVarFlags>(static_cast<u32>(a) | static_cast<u32>(b));
    }
    
    inline CVarFlags operator&(CVarFlags a, CVarFlags b)
    {
        return static_cast<CVarFlags>(static_cast<u32>(a) & static_cast<u32>(b));
    }
    
    inline bool HasFlag(CVarFlags flags, CVarFlags flag)
    {
        return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
    }
    
    using CVarValue = std::variant<bool, i32, f32, std::string>;
    
    class CVar
    {
    public:
        CVar(const std::string& name, bool defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar(const std::string& name, i32 defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar(const std::string& name, f32 defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar(const std::string& name, const std::string& defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        
        const std::string& GetName() const { return m_Name; }
        const std::string& GetDescription() const { return m_Description; }
        CVarFlags GetFlags() const { return m_Flags; }
        
        bool GetBool() const;
        i32 GetInt() const;
        f32 GetFloat() const;
        const std::string& GetString() const;
        
        void SetBool(bool value);
        void SetInt(i32 value);
        void SetFloat(f32 value);
        void SetString(const std::string& value);
        
        void SetFromString(const std::string& value);
        std::string GetAsString() const;
        
        void Reset();
        
        using ChangeCallback = std::function<void(const CVar&)>;
        void AddChangeCallback(ChangeCallback callback);
        
    private:
        void NotifyChange();
        
        std::string m_Name;
        std::string m_Description;
        CVarFlags m_Flags;
        CVarValue m_Value;
        CVarValue m_DefaultValue;
        std::vector<ChangeCallback> m_ChangeCallbacks;
    };
    
    class CVarSystem
    {
    public:
        static CVarSystem& Get();
        
        CVar* Register(const std::string& name, bool defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar* Register(const std::string& name, i32 defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar* Register(const std::string& name, f32 defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        CVar* Register(const std::string& name, const std::string& defaultValue, const std::string& description = "", CVarFlags flags = CVarFlags::None);
        
        CVar* Find(const std::string& name);
        const CVar* Find(const std::string& name) const;
        
        bool Exists(const std::string& name) const;
        
        void SetFromString(const std::string& name, const std::string& value);
        std::string GetAsString(const std::string& name) const;
        
        void LoadFromFile(const std::string& filepath);
        void SaveToFile(const std::string& filepath) const;
        
        void ExecuteCommand(const std::string& command);
        
        std::vector<std::string> GetAllCVarNames() const;
        std::vector<std::string> AutoComplete(const std::string& prefix) const;
        
    private:
        CVarSystem() = default;
        
        std::unordered_map<std::string, Scope<CVar>> m_CVars;
    };
    
    // Helper macros for declaring CVars
    #define FORCE_CVAR_BOOL(name, default, desc) \
        static CVar* name = CVarSystem::Get().Register(#name, default, desc)
    
    #define FORCE_CVAR_INT(name, default, desc) \
        static CVar* name = CVarSystem::Get().Register(#name, default, desc)
    
    #define FORCE_CVAR_FLOAT(name, default, desc) \
        static CVar* name = CVarSystem::Get().Register(#name, default, desc)
    
    #define FORCE_CVAR_STRING(name, default, desc) \
        static CVar* name = CVarSystem::Get().Register(#name, std::string(default), desc)
}

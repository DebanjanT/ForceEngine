#pragma once

#include "Force/Core/Base.h"
#include <unordered_map>
#include <vector>

namespace Force
{
    struct ModuleInfo
    {
        std::string Name;
        std::string Version;
        std::string Author;
        std::string Description;
        std::vector<std::string> Dependencies;
    };
    
    class Module
    {
    public:
        virtual ~Module() = default;
        
        virtual void OnLoad() {}
        virtual void OnUnload() {}
        virtual void OnUpdate(f32 deltaTime) {}
        
        virtual const ModuleInfo& GetInfo() const = 0;
        
        bool IsLoaded() const { return m_Loaded; }
        
    protected:
        friend class ModuleManager;
        bool m_Loaded = false;
    };
    
    // Module factory function type
    using ModuleCreateFunc = Module* (*)();
    using ModuleDestroyFunc = void (*)(Module*);
    
    // Macro for defining module entry points
    #define FORCE_MODULE(ModuleClass) \
        extern "C" FORCE_API Module* CreateModule() { return new ModuleClass(); } \
        extern "C" FORCE_API void DestroyModule(Module* module) { delete module; }
    
    class ModuleManager
    {
    public:
        static ModuleManager& Get();
        
        // Load a module from a dynamic library
        bool LoadModule(const std::string& path);
        
        // Unload a module
        void UnloadModule(const std::string& name);
        
        // Get a loaded module by name
        Module* GetModule(const std::string& name);
        
        // Check if a module is loaded
        bool IsModuleLoaded(const std::string& name) const;
        
        // Get all loaded modules
        const std::unordered_map<std::string, Module*>& GetLoadedModules() const { return m_Modules; }
        
        // Update all modules
        void UpdateAll(f32 deltaTime);
        
        // Unload all modules
        void UnloadAll();
        
    private:
        ModuleManager() = default;
        ~ModuleManager() { UnloadAll(); }
        
        struct LoadedModule
        {
            void* LibraryHandle = nullptr;
            Module* Instance = nullptr;
            ModuleDestroyFunc DestroyFunc = nullptr;
        };
        
        std::unordered_map<std::string, Module*> m_Modules;
        std::unordered_map<std::string, LoadedModule> m_LoadedModules;
    };
    
    // Built-in module registration (for statically linked modules)
    class ModuleRegistry
    {
    public:
        static ModuleRegistry& Get();
        
        template<typename T>
        void Register(const std::string& name)
        {
            m_Factories[name] = []() -> Module* { return new T(); };
        }
        
        Module* Create(const std::string& name);
        bool HasModule(const std::string& name) const;
        
        std::vector<std::string> GetRegisteredModules() const;
        
    private:
        ModuleRegistry() = default;
        
        std::unordered_map<std::string, ModuleCreateFunc> m_Factories;
    };
    
    // Helper macro for static module registration
    #define FORCE_REGISTER_MODULE(ModuleClass, ModuleName) \
        namespace { \
            struct ModuleClass##Registrar { \
                ModuleClass##Registrar() { \
                    Force::ModuleRegistry::Get().Register<ModuleClass>(ModuleName); \
                } \
            }; \
            static ModuleClass##Registrar s_##ModuleClass##Registrar; \
        }
}

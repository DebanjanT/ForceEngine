#include "Force/Core/Module.h"
#include "Force/Platform/Platform.h"
#include "Force/Core/Logger.h"

namespace Force
{
    ModuleManager& ModuleManager::Get()
    {
        static ModuleManager instance;
        return instance;
    }
    
    bool ModuleManager::LoadModule(const std::string& path)
    {
        void* library = Platform::LoadDynamicLibrary(path);
        if (!library)
        {
            FORCE_CORE_ERROR("Failed to load module: {}", path);
            return false;
        }
        
        auto createFunc = reinterpret_cast<ModuleCreateFunc>(
            Platform::GetDynamicLibraryFunction(library, "CreateModule"));
        auto destroyFunc = reinterpret_cast<ModuleDestroyFunc>(
            Platform::GetDynamicLibraryFunction(library, "DestroyModule"));
        
        if (!createFunc || !destroyFunc)
        {
            FORCE_CORE_ERROR("Module {} does not export required functions", path);
            Platform::UnloadDynamicLibrary(library);
            return false;
        }
        
        Module* module = createFunc();
        if (!module)
        {
            FORCE_CORE_ERROR("Failed to create module instance: {}", path);
            Platform::UnloadDynamicLibrary(library);
            return false;
        }
        
        const ModuleInfo& info = module->GetInfo();
        
        // Check dependencies
        for (const auto& dep : info.Dependencies)
        {
            if (!IsModuleLoaded(dep))
            {
                FORCE_CORE_ERROR("Module {} requires dependency {} which is not loaded", info.Name, dep);
                destroyFunc(module);
                Platform::UnloadDynamicLibrary(library);
                return false;
            }
        }
        
        LoadedModule loaded;
        loaded.LibraryHandle = library;
        loaded.Instance = module;
        loaded.DestroyFunc = destroyFunc;
        
        m_LoadedModules[info.Name] = loaded;
        m_Modules[info.Name] = module;
        
        module->OnLoad();
        module->m_Loaded = true;
        
        FORCE_CORE_INFO("Loaded module: {} v{}", info.Name, info.Version);
        
        return true;
    }
    
    void ModuleManager::UnloadModule(const std::string& name)
    {
        auto it = m_LoadedModules.find(name);
        if (it == m_LoadedModules.end())
        {
            FORCE_CORE_WARN("Module {} is not loaded", name);
            return;
        }
        
        LoadedModule& loaded = it->second;
        
        if (loaded.Instance->m_Loaded)
        {
            loaded.Instance->OnUnload();
            loaded.Instance->m_Loaded = false;
        }
        
        loaded.DestroyFunc(loaded.Instance);
        Platform::UnloadDynamicLibrary(loaded.LibraryHandle);
        
        m_Modules.erase(name);
        m_LoadedModules.erase(it);
        
        FORCE_CORE_INFO("Unloaded module: {}", name);
    }
    
    Module* ModuleManager::GetModule(const std::string& name)
    {
        auto it = m_Modules.find(name);
        return it != m_Modules.end() ? it->second : nullptr;
    }
    
    bool ModuleManager::IsModuleLoaded(const std::string& name) const
    {
        return m_Modules.find(name) != m_Modules.end();
    }
    
    void ModuleManager::UpdateAll(f32 deltaTime)
    {
        for (auto& [name, module] : m_Modules)
        {
            if (module->m_Loaded)
            {
                module->OnUpdate(deltaTime);
            }
        }
    }
    
    void ModuleManager::UnloadAll()
    {
        std::vector<std::string> moduleNames;
        for (const auto& [name, module] : m_Modules)
        {
            moduleNames.push_back(name);
        }
        
        // Unload in reverse order
        for (auto it = moduleNames.rbegin(); it != moduleNames.rend(); ++it)
        {
            UnloadModule(*it);
        }
    }
    
    // ModuleRegistry
    ModuleRegistry& ModuleRegistry::Get()
    {
        static ModuleRegistry instance;
        return instance;
    }
    
    Module* ModuleRegistry::Create(const std::string& name)
    {
        auto it = m_Factories.find(name);
        if (it != m_Factories.end())
        {
            return it->second();
        }
        return nullptr;
    }
    
    bool ModuleRegistry::HasModule(const std::string& name) const
    {
        return m_Factories.find(name) != m_Factories.end();
    }
    
    std::vector<std::string> ModuleRegistry::GetRegisteredModules() const
    {
        std::vector<std::string> names;
        for (const auto& [name, factory] : m_Factories)
        {
            names.push_back(name);
        }
        return names;
    }
}

# Force Engine

A modern C++20 game engine built with Vulkan 1.3.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red.svg)

## Features

### Phase 1: Foundation ✅
- **Build System**: CMake-based cross-platform build
- **Platform Layer**: GLFW window management with platform abstraction
- **Vulkan Renderer**: Modern Vulkan 1.3 rendering backend
- **Math Library**: GLM integration with custom type aliases
- **Memory Management**: Custom allocators (Linear, Pool, Frame) + VMA
- **Logging**: spdlog-based logging system
- **Profiling**: Tracy profiler integration (optional)

### Phase 2: Engine Core ✅
- **Subsystem Registry**: Priority-based system management
- **Event Bus**: Type-safe publish/subscribe pattern
- **ECS**: EnTT entity-component-system integration
- **Module System**: Dynamic plugin loading support
- **CVars**: Console variable system with config files

## Architecture

```
ForceEngine/
├── Engine/           # Core engine library (ForceEngine.lib)
│   ├── Include/      # Public headers
│   └── Source/       # Implementation
├── Editor/           # Editor executable (ForceEditor.exe)
├── Runtime/          # Shipping runtime (ForceRuntime.exe)
├── External/         # Third-party dependencies (auto-cloned)
├── Assets/           # Editor assets
├── Scripts/          # Build & setup scripts
└── Build/            # Generated build files (gitignored)
```

## Prerequisites

Before running the setup script, you need to install:

### 1. Git
Download and install from: https://git-scm.com/download/win

### 2. CMake (3.21+)
Download and install from: https://cmake.org/download/
> ⚠️ **Important**: Check "Add CMake to PATH" during installation

### 3. Vulkan SDK (1.3+)
Download and install from: https://vulkan.lunarg.com/sdk/home
> ⚠️ **Important**: Restart your computer after installation to set environment variables

### 4. Visual Studio 2022
Download Community edition from: https://visualstudio.microsoft.com/
> ⚠️ **Important**: Install "Desktop development with C++" workload

## Quick Start

### 1. Clone the Repository
```cmd
git clone https://github.com/yourusername/ForceEngine.git
cd ForceEngine
```

### 2. Run Setup Script
```cmd
Scripts\Setup.bat
```

This will automatically:
- ✅ Check all prerequisites (Git, CMake, Vulkan SDK, Visual Studio)
- ✅ Clone all dependencies (GLFW, GLM, spdlog, VMA, Tracy, EnTT)
- ✅ Generate Visual Studio 2022 solution

### 3. Build
**Option A: Visual Studio**
1. Open `Build\ForceEngine.sln`
2. Set `ForceEditor` as startup project
3. Press F5 to build and run

**Option B: Command Line**
```cmd
Scripts\Build.bat Debug
```

### 4. Run
```cmd
Scripts\Run.bat Debug
```

## Scripts

| Script | Description |
|--------|-------------|
| `Scripts\Setup.bat` | First-time setup - clones dependencies and generates project |
| `Scripts\Build.bat [Debug/Release]` | Build the project |
| `Scripts\Run.bat [Debug/Release]` | Run the editor |
| `Scripts\Clean.bat` | Remove build directory |
| `Scripts\Regenerate.bat` | Regenerate CMake project |

## Dependencies

All dependencies are automatically cloned by the setup script:

| Library | Version | Purpose | License |
|---------|---------|---------|---------|
| [Vulkan SDK](https://vulkan.lunarg.com/) | 1.3+ | Graphics API | Apache 2.0 |
| [GLFW](https://github.com/glfw/glfw) | 3.3+ | Window/Input | zlib |
| [GLM](https://github.com/g-truc/glm) | 0.9.9+ | Math | MIT |
| [spdlog](https://github.com/gabime/spdlog) | 1.x | Logging | MIT |
| [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | 3.x | GPU Memory | MIT |
| [Tracy](https://github.com/wolfpld/tracy) | 0.9+ | Profiling | BSD-3 |
| [EnTT](https://github.com/skypjack/entt) | 3.x | ECS | MIT |

## Build Options

Configure in CMake or via command line:

| Option | Default | Description |
|--------|---------|-------------|
| `FORCE_BUILD_EDITOR` | ON | Build editor executable |
| `FORCE_BUILD_RUNTIME` | ON | Build runtime executable |
| `FORCE_ENABLE_PROFILING` | OFF | Enable Tracy profiling |
| `FORCE_ENABLE_VALIDATION` | ON | Enable Vulkan validation layers |

## Troubleshooting

### "Vulkan SDK not found"
- Make sure Vulkan SDK is installed
- Restart your computer after installation
- Check that `VULKAN_SDK` environment variable is set

### "CMake not found"
- Reinstall CMake and check "Add to PATH"
- Or add CMake bin directory to PATH manually

### Build errors about missing headers
- Run `Scripts\Setup.bat` again
- Make sure all dependencies were cloned successfully

### Black screen / Validation errors
- Update your GPU drivers
- Make sure your GPU supports Vulkan 1.3

## Roadmap

### Phase 1: Foundation ✅
- [x] Build system (CMake)
- [x] Platform abstraction (GLFW)
- [x] Vulkan bootstrap
- [x] Math library (GLM)
- [x] Memory management
- [x] Logging & profiling

### Phase 2: Engine Core ✅
- [x] Subsystem registry
- [x] Event bus
- [x] ECS (EnTT)
- [x] Module/Plugin system
- [x] Config & CVars

### Phase 3: Rendering (Next)
- [ ] Render graph
- [ ] PBR renderer
- [ ] Shader system
- [ ] Static mesh rendering
- [ ] Debug renderer

### Phase 4: Scene & Assets
- [ ] Scene graph
- [ ] Asset system
- [ ] Material system
- [ ] Texture streaming

### Phase 5+
See full roadmap in documentation.

## License

MIT License - See [LICENSE](LICENSE) file

## Contributing

Contributions welcome! Please read CONTRIBUTING.md first.

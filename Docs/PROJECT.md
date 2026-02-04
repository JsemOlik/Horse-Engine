# Horse Engine - Project Documentation

**Version:** 0.1.0  
**Status:** ✅ Phase 1 Complete (Bootstrap and Foundations)  
**Build System:** CMake + Ninja + vcpkg  
**IDE:** CLion (or any CMake-compatible IDE)

---

## Quick Start (5 Minutes)

### Prerequisites

1. **Windows 10/11 (x64)**
2. **CMake 3.21+** - [Download](https://cmake.org/download/)
3. **Ninja** - Included with CLion or install separately
4. **MSVC Compiler** - Install Build Tools for Visual Studio 2022
5. **vcpkg** - Package manager

### Setup vcpkg

```powershell
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap
.\bootstrap-vcpkg.bat

# Set environment variable
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')
```

### Build in CLion

1. Open CLion
2. Open Folder → `C:\Users\olik\Desktop\Coding\Horse-Engine`
3. CLion will auto-detect CMakePresets.json
4. Select preset: **Debug** (or DevRelease/Shipping)
5. Build → Build All (Ctrl+F9)
6. Run → HorseEditor

### Build via Command Line

```powershell
# Configure
cmake --preset Debug

# Build
cmake --build Build/Debug

# Run Editor
.\Build\Debug\Editor\HorseEditor.exe
```

---

## Project Structure

```
Horse-Engine/
├── Engine/
│   ├── Runtime/              # Core engine (NO Qt dependencies)
│   │   ├── Include/HorseEngine/
│   │   │   ├── Core.h                    # Common types (u32, f32, etc.)
│   │   │   ├── Engine.h                  # Main engine class
│   │   │   ├── Core/
│   │   │   │   ├── Logging.h            # Async logging (spdlog)
│   │   │   │   ├── Time.h               # High-res timer, FPS
│   │   │   │   ├── Memory.h             # Linear/frame allocators
│   │   │   │   └── JobSystem.h          # Thread pool
│   │   │   └── Platform/
│   │   │       └── Win32Window.h        # Window management
│   │   └── Source/
│   │       ├── Engine.cpp
│   │       ├── Core/                     # Implementation
│   │       └── Platform/                 # Win32 implementation
│   │
│   ├── RenderD3D11/          # Direct3D 11 renderer module
│   │   ├── Include/HorseEngine/Render/
│   │   │   ├── D3D11Renderer.h          # Device/SwapChain
│   │   │   └── D3D11Shader.h            # Shader compiler
│   │   └── Source/
│   │
│   ├── Scripting/            # (Phase 6) Lua + C++ gameplay
│   └── Physics/              # (Phase 8) Jolt/PhysX
│
├── Editor/                   # Qt5 editor application
│   └── Source/
│       ├── main.cpp                      # Entry point
│       ├── EditorWindow.h/cpp            # Main window
│       ├── D3D11ViewportWidget.h/cpp     # Embedded D3D11 viewport
│       └── Panels/                       # Dockable panels
│           ├── HierarchyPanel.h/cpp
│           ├── InspectorPanel.h/cpp
│           ├── ContentBrowserPanel.h/cpp
│           └── ConsolePanel.h/cpp
│
├── Tools/
│   ├── Cooker/               # (Phase 4) Asset cooker
│   └── Packager/             # (Phase 7) Game packager
│
├── Docs/
├── ThirdParty/               # Optional vendor drops
└── Build/                    # Build output (git-ignored)
```

---

## Architecture Overview

### Engine/Runtime - Core Systems

**Zero Qt dependencies.** All core systems live here:

#### Platform Layer
- **Win32Window** - Window creation, message loop, resize handling
- HWND exposure for D3D11 integration
- DPI awareness, VSync toggle

#### Core Systems

**Logging (`Core/Logging.h`)**
```cpp
HORSE_LOG_CORE_INFO("Engine initialized");
HORSE_LOG_RENDER_ERROR("Failed: {}", error);
```
- 4 channels: Core, Render, Asset, Script
- Async sinks (console + file)
- spdlog-based

**Time (`Core/Time.h`)**
```cpp
Time::Initialize();
Time::Update();
float dt = Time::GetDeltaTime();
float fps = Time::GetFPS();
```
- QueryPerformanceCounter-based
- Delta time tracking
- FPS calculation

**Memory (`Core/Memory.h`)**
```cpp
FrameAllocator::Initialize();
void* ptr = FrameAllocator::Allocate(size);
FrameAllocator::Reset(); // Per-frame
```
- LinearAllocator: Sequential allocation
- FrameAllocator: 16 MB transient per-frame memory

**Job System (`Core/JobSystem.h`)**
```cpp
JobSystem::Initialize();
JobSystem::Execute([] { /* work */ });
auto future = JobSystem::ExecuteAsync([] { return 42; });
```
- Thread pool with work-stealing queue
- Auto-detects CPU cores

### RenderD3D11 - Direct3D 11 Renderer

**D3D11Renderer (`Render/D3D11Renderer.h`)**
```cpp
D3D11Renderer renderer;
RendererDesc desc;
desc.WindowHandle = hwnd;
renderer.Initialize(desc);
renderer.Clear(0.2f, 0.3f, 0.4f);
renderer.Present();
```
- Device/SwapChain creation
- Feature level checks (11.0+)
- Render target management
- Resize handling
- VSync support

**D3D11Shader (`Render/D3D11Shader.h`)**
- D3DCompile wrapper
- Vertex/Pixel shader compilation
- Error reporting

### Editor - Qt5 Application

**EditorWindow**
- QMainWindow with menu bar (File/Edit/View/Help)
- Dockable panels system
- Toolbar (Play/Pause/Stop placeholders)

**D3D11ViewportWidget**
- Embeds native HWND in Qt widget
- D3D11 renderer integration
- 60 FPS timer-based rendering

**Panels (Placeholders for Phase 2)**
- Hierarchy - Scene entity tree
- Inspector - Component properties
- Content Browser - Asset management
- Console - Log output

---

## CMake Build System

### Presets

Three build configurations in `CMakePresets.json`:

1. **Debug** - Full debug info, no optimizations
2. **DevRelease** - O2 optimization + debug info
3. **Shipping** - Full optimization + LTO

### Features

- **Generator:** Ninja (fast incremental builds)
- **Compiler:** MSVC (cl.exe)
- **Unity Builds:** Enabled for faster compilation
- **Warnings:** /W4, /permissive-, treat warnings as errors
- **vcpkg:** Automatic dependency management

### CLion Configuration

CLion auto-detects `CMakePresets.json`. Select preset from:
- Settings → Build, Execution, Deployment → CMake → Profiles

---

## Dependencies (vcpkg.json)

| Package | Purpose | Phase |
|---------|---------|-------|
| spdlog | Logging | 1 ✅ |
| fmt | String formatting | 1 ✅ |
| entt | Entity Component System | 2 |
| qt5-base | Editor UI | 1 ✅ |
| stb | Image loading | 3 |
| assimp | Mesh importing | 4 |
| luajit | Scripting runtime | 6 |
| sol2 | Lua bindings | 6 |
| physfs | Virtual filesystem | 7 |
| zlib | Compression | 7 |

---

## Phase 1 - Complete ✅

### What's Implemented

#### Toolchain
- ✅ CMake presets (Debug/DevRelease/Shipping)
- ✅ Ninja generator for CLion
- ✅ vcpkg integration
- ✅ Unity builds, LTO, warnings-as-errors

#### Engine/Runtime
- ✅ Win32 platform layer (window, message loop)
- ✅ Logging system (spdlog, 4 channels, async)
- ✅ Time system (high-res timer, FPS)
- ✅ Memory allocators (linear, frame)
- ✅ Job system (thread pool, work-stealing)

#### RenderD3D11
- ✅ D3D11 device/context/swap chain
- ✅ Feature level checks
- ✅ Shader compilation pipeline
- ✅ Viewport management
- ✅ Clear color + Present

#### Editor
- ✅ Qt5 Widgets application
- ✅ Main window with menus
- ✅ Dockable panels (4 placeholders)
- ✅ D3D11 viewport widget
- ✅ 60 FPS rendering

#### Documentation
- ✅ Complete README.md
- ✅ This PROJECT.md file

### Deliverable

✅ Qt5 Editor opens successfully  
✅ Shows D3D11 viewport with blue clear color  
✅ Engine and Editor build as separate targets  
✅ Complete documentation  

**Phase 1 Complete!** Ready for Phase 2.

---

## File Index

### Core Headers (Engine/Runtime/Include)

| File | Description | Key Features |
|------|-------------|--------------|
| `Core.h` | Common types, platform defines | u32, f32, etc. |
| `Engine.h` | Main engine class | Init/Shutdown/Run |
| `Core/Logging.h` | Logging system | 4 channels, async |
| `Core/Time.h` | Time system | Delta time, FPS |
| `Core/Memory.h` | Allocators | Linear, frame |
| `Core/JobSystem.h` | Thread pool | Work-stealing |
| `Platform/Win32Window.h` | Window management | HWND, messages |

### Render Headers (Engine/RenderD3D11/Include)

| File | Description |
|------|-------------|
| `Render/D3D11Renderer.h` | D3D11 renderer |
| `Render/D3D11Shader.h` | Shader compiler |

### Editor Files (Editor/Source)

| File | Description |
|------|-------------|
| `main.cpp` | Qt application entry |
| `EditorWindow.h/cpp` | Main window |
| `D3D11ViewportWidget.h/cpp` | Viewport widget |
| `Panels/HierarchyPanel.h/cpp` | Entity tree |
| `Panels/InspectorPanel.h/cpp` | Properties |
| `Panels/ContentBrowserPanel.h/cpp` | Assets |
| `Panels/ConsolePanel.h/cpp` | Console |

---

## Usage Examples

### Initialize Engine

```cpp
#include "HorseEngine/Engine.h"

int main() {
    Horse::Engine engine;
    if (engine.Initialize()) {
        engine.Run();
    }
    engine.Shutdown();
    return 0;
}
```

### Logging

```cpp
#include "HorseEngine/Core/Logging.h"

HORSE_LOG_CORE_INFO("Starting engine...");
HORSE_LOG_RENDER_ERROR("Failed to create device: {}", hr);
HORSE_LOG_CORE_WARN("Asset not found: {}", path);
```

### Time

```cpp
#include "HorseEngine/Core/Time.h"

void GameLoop() {
    while (running) {
        Horse::Time::Update();
        
        float dt = Horse::Time::GetDeltaTime();
        float fps = Horse::Time::GetFPS();
        
        Update(dt);
        Render();
    }
}
```

### Memory

```cpp
#include "HorseEngine/Core/Memory.h"

void FrameUpdate() {
    Horse::FrameAllocator::Reset();
    
    void* tempData = Horse::FrameAllocator::Allocate(1024);
    // Use tempData for this frame
    // Automatically freed next frame
}
```

### Job System

```cpp
#include "HorseEngine/Core/JobSystem.h"

// Fire and forget
Horse::JobSystem::Execute([] {
    LoadAssetAsync("mesh.gltf");
});

// Get result
auto future = Horse::JobSystem::ExecuteAsync([] {
    return CalculateLighting();
});
auto result = future.get();
```

### D3D11 Rendering

```cpp
#include "HorseEngine/Render/D3D11Renderer.h"

Horse::D3D11Renderer renderer;

Horse::RendererDesc desc;
desc.WindowHandle = hwnd;
desc.Width = 1280;
desc.Height = 720;
desc.VSync = true;

renderer.Initialize(desc);

while (running) {
    renderer.BeginFrame();
    renderer.Clear(0.2f, 0.3f, 0.4f);
    
    // Render scene here
    
    renderer.EndFrame();
    renderer.Present();
}
```

---

## Development Guidelines

### Code Style

- C++20 features encouraged
- Minimal comments (self-documenting code)
- Type aliases: `u32`, `f32`, etc.
- RAII everywhere
- Performance-first mindset

### Adding a New System

1. Create header in `Engine/Runtime/Include/HorseEngine/`
2. Create implementation in `Engine/Runtime/Source/`
3. Add to `Engine/Runtime/CMakeLists.txt`
4. Include in `Engine.h` if part of core

### Modifying Editor UI

1. Edit files in `Editor/Source/Panels/`
2. Rebuild (CLion handles MOC automatically)
3. Qt's Meta-Object Compiler processes Q_OBJECT classes

### Update Policy

**Always update README.md after significant changes:**
1. What changed (1-2 lines)
2. How to use it (3-6 steps or code snippet)
3. Migration notes (breaking changes)
4. Link to deeper docs in /Docs

---

## Troubleshooting

### vcpkg not found
```powershell
# Set environment variable
$env:VCPKG_ROOT
# Should output: C:\vcpkg

# If not set:
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')
```

### Qt5 not found
```powershell
cd $env:VCPKG_ROOT
.\vcpkg install qt5-base:x64-windows
```

### Ninja not found
- Install via CLion: Settings → Build → Toolchains
- Or download: https://github.com/ninja-build/ninja/releases

### D3D11 Debug Layer errors
- Windows Settings → Apps → Optional Features → Graphics Tools

### Build errors
```powershell
# Clean rebuild
Remove-Item -Recurse -Force Build
cmake --preset Debug
cmake --build Build/Debug
```

### CLion not detecting presets
- File → Reload CMake Project
- Settings → Build → CMake → Reload CMake Project

---

## Performance Targets

- Editor opens medium scene in <2s on SSD
- 10k static meshes at 120 FPS (with culling, Phase 3+)
- 500 dynamic bodies at 60+ FPS (Phase 8+)

---

## Statistics

| Metric | Count |
|--------|-------|
| Directories | 25 |
| C++ Headers | 15 |
| C++ Sources | 16 |
| CMake Files | 4 |
| Total Files | 41 |
| Lines of Code | ~3,500 |

---

## Roadmap

- [x] **Phase 1** - Bootstrap and foundations ✅
- [ ] **Phase 2** - ECS, scenes, serialization
- [ ] **Phase 3** - Renderer core and materials
- [ ] **Phase 4** - Asset pipeline and cooker
- [ ] **Phase 5** - Level system and PIE
- [ ] **Phase 6** - Game coding system (Lua + C++ DLL)
- [ ] **Phase 7** - Packaging (exe + pak + dll)
- [ ] **Phase 8** - Physics and navigation
- [ ] **Phase 9** - Lighting and polish

See [TODO_LIST.md](../TODO_LIST.md) for complete roadmap (340 lines).

---

## Next Phase - Phase 2

**ECS, Scenes, and Serialization**

Will add:
- EnTT entity component system
- Components: Transform, MeshRenderer, Camera, Light
- JSON scene serialization (.horselevel.json)
- Binary runtime formats (.horselevel)
- Entity hierarchy in editor
- Inspector panel wired up
- Save/Load functionality

---

**Last Updated:** Phase 1 Complete - 2026-02-04

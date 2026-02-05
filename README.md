# Horse Engine

A high-performance 3D game engine built with C++20, Direct3D 11, and Qt5.

**Version:** 0.1.0 (Phase 1 - Bootstrap Complete)

---

## Features (Phase 1)

- ✅ **Clean Architecture**: Strict separation between runtime engine and editor
- ✅ **Modern C++20**: Standards-compliant, high-performance codebase
- ✅ **Direct3D 11 Renderer**: Hardware-accelerated graphics with feature level 11.0+
- ✅ **Qt5 Editor**: Professional editor UI with dockable panels
- ✅ **Core Systems**: Logging, time, memory allocators, job system
- ✅ **CMake Build System**: Cross-configuration presets (Debug/DevRelease/Shipping)
- ✅ **Package Management**: vcpkg integration for dependencies

---

## Getting Started

### Prerequisites

1. **Windows 10/11 (x64)** - Required
2. **CLion** - JetBrains IDE (or any CMake-compatible IDE)
3. **MSVC Compiler** - Install Build Tools for Visual Studio 2022
4. **CMake 3.21+** - Included with CLion
5. **Ninja** - Included with CLion
6. **vcpkg** - Package manager for C++ libraries

### Installing vcpkg

```powershell
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable
[System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')
```

### Building the Engine

**In CLion:**

1. Open CLion
2. Open Folder → Select `Horse-Engine` directory
3. CLion will auto-detect `CMakePresets.json`
4. Select preset: **Debug** (in CMake tool window)
5. Build → Build All (Ctrl+F9)
6. Run → HorseEditor

**Via Command Line:**

```powershell
# Clone the repository
git clone https://github.com/JsemOlik/Horse-Engine.git Horse-Engine
cd Horse-Engine

# Configure with CMake (Debug preset)
cmake --preset Debug

# Build with Ninja
cmake --build Build/Debug

# Or use DevRelease for optimized builds with debug info
cmake --preset DevRelease
cmake --build Build/DevRelease
```

### Running the Editor

```powershell
# After building, run the editor
.\Build\Debug\Editor\HorseEditor.exe

# Or for DevRelease
.\Build\DevRelease\Editor\HorseEditor.exe
```

### Asset Cooking

The Asset Cooker is a standalone tool used to convert authoring assets (textures, meshes, materials, levels) into optimized binary formats for runtime.

```powershell
# Run the cooker for a project
# Usage: HorseCooker <AssetsDir> <OutputDir> [Platform]
.\Build\Debug\Tools\HorseCooker.exe .\MyProject\Assets .\MyProject\Cooked Windows
```

The cooker produces:

- Individual cooked binary files (e.g., `.horsetex`, `.horsemat.bin`)
- `Game.manifest.json`: A mapping of asset GUIDs to cooked file paths
- `Game.project.bin`: Optimized project settings file

---

## Project Structure

```
Horse-Engine/
├── Engine/
│   ├── Runtime/          # Core engine (no UI/Qt dependencies)
│   │   ├── Include/      # Public headers
│   │   └── Source/       # Implementation
│   ├── RenderD3D11/      # Direct3D 11 renderer module
│   ├── Scripting/        # Lua + C++ gameplay bridge (Phase 6)
│   └── Physics/          # Physics backend (Phase 8)
├── Editor/               # Qt5-based editor application
│   └── Source/           # Editor source code
│       └── Panels/       # Editor UI panels
├── Tools/
│   ├── Cooker/           # Asset cooker (Phase 4)
│   └── Packager/         # Game packager (Phase 7)
├── Docs/                 # Documentation ([Lua Scripting](Docs/Lua-Scripting.md))
├── ThirdParty/           # Optional vendor drops
└── Build/                # Build output (git-ignored)
```

---

## CMake Presets

Three build configurations (using Ninja generator for CLion):

1. **Debug** - Full debug info, no optimizations, unity builds enabled

   ```powershell
   cmake --preset Debug
   cmake --build Build/Debug
   ```

2. **DevRelease** - Optimized with debug info for development

   ```powershell
   cmake --preset DevRelease
   cmake --build Build/DevRelease
   ```

3. **Shipping** - Fully optimized, LTO enabled, for final builds
   ```powershell
   cmake --preset Shipping
   cmake --build Build/Shipping
   ```

**In CLion:** Select preset from Settings → Build, Execution, Deployment → CMake → Profiles

---

## Engine Architecture

### Runtime (Engine/Runtime)

Zero Qt dependencies. Contains:

- **Platform Layer** (`Platform/Win32Window.h`)
  - Win32 window creation and message loop
  - Raw input handling
  - DPI awareness
  - VSync toggle

- **Core Systems** (`Core/`)
  - **Logging** (`Logging.h`) - spdlog-based async logging with channels (Core, Render, Asset, Script)
  - **Time** (`Time.h`) - High-resolution timer, delta time, FPS tracking
  - **Memory** (`Memory.h`) - Linear and frame allocators for cache-friendly allocations
  - **Job System** (`JobSystem.h`) - Thread pool with work-stealing for async tasks

### RenderD3D11 (Engine/RenderD3D11)

Direct3D 11 rendering module:

- Device and swap chain creation
- Render target management
- Shader compilation pipeline (D3DCompiler)
- Viewport and resize handling
- Debug layer support

### Editor (Editor/)

Qt5 Widgets application:

- **Main Window** - Menu bar, toolbar, dockable panels
- **Viewport Widget** - Embeds D3D11 rendering surface
- **Panels**:
  - Hierarchy - Scene entity tree (placeholder)
  - Inspector - Entity properties (placeholder)
  - Content Browser - Asset management (placeholder)
  - Console - Log output (placeholder)

---

## Current Phase Status

✅ **Phase 1 Complete - Bootstrap and Foundations**

What's working:

- CMake build system with presets
- vcpkg package management
- Engine/Runtime with Win32 platform layer
- Logging system with async sinks
- High-resolution time system
- Memory allocators (linear, frame)
- Job system with thread pool
- D3D11 renderer initialization
- Qt5 Editor with embedded D3D11 viewport
- Editor UI with dockable panels

What's next (Phase 2):

- ECS with EnTT
- JSON scene serialization
- Binary runtime formats
- Entity hierarchy and components

---

## Dependencies

Managed via vcpkg (`vcpkg.json`):

- **spdlog** - Fast logging library
- **fmt** - String formatting
- **entt** - Entity Component System
- **qt5-base** - Qt5 Widgets for editor
- **stb** - Image loading (Phase 3+)
- **assimp** - Mesh importing (Phase 4+)
- **luajit** - Scripting runtime (Phase 6+)
- **sol2** - Lua bindings (Phase 6+)
- **physfs** - Virtual filesystem (Phase 7+)
- **zlib** - Compression (Phase 7+)

---

## How To...

### Add a New System to Runtime

1. Create header in `Engine/Runtime/Include/HorseEngine/`
2. Create implementation in `Engine/Runtime/Source/`
3. Add to `Engine/Runtime/CMakeLists.txt`
4. Include in `Engine.h` if part of core

### Modify the Editor UI

1. Edit panel files in `Editor/Source/Panels/`
2. Rebuild Editor target
3. Qt's MOC will automatically process Q_OBJECT classes

### Change Clear Color

Edit `D3D11ViewportWidget.cpp`:

```cpp
float m_ClearColor[3] = {0.2f, 0.3f, 0.4f}; // RGB values
```

### View Logs

- Console output: See terminal/Visual Studio output window
- File output: Check `HorseEngine.log` in working directory

### Script with Lua

For a complete guide on how to use Lua in the Horse Engine, see the [Lua Scripting Documentation](Docs/Lua-Scripting.md).

### Cook Assets for Distribution

1. Build the **HorseCooker** target.
2. Open a terminal and navigate to the build output directory.
3. Run `HorseCooker.exe <AssetsPath> <OutputPath> <Platform>`.
4. Ensure the generated `Game.manifest.json` and cooked assets are included in your runtime distribution.

---

## Development Guidelines

### Update Policy

**Always update this README after significant changes:**

1. **What changed** - 1-2 line summary
2. **How to use it** - 3-6 bullet steps or code snippet
3. **Migration notes** - Breaking API changes
4. **Link to docs** - Reference `/Docs` for detailed specs

### Code Style

- C++20 features encouraged
- Minimal comments (self-documenting code preferred)
- Use type aliases: `u32`, `f32`, etc.
- RAII everywhere
- Performance-first mindset

### Logging Macros

```cpp
HORSE_LOG_CORE_INFO("Engine initialized");
HORSE_LOG_RENDER_ERROR("Failed to create device: {}", errorCode);
HORSE_LOG_CORE_WARN("Missing asset: {}", assetPath);
```

---

## Performance Targets

- Editor opens medium scene in <2 seconds on SSD
- 10,000 static meshes at 120 FPS (with culling, Phase 3+)
- 500 dynamic physics bodies at 60+ FPS (Phase 8+)

---

## Roadmap

- [x] **Phase 1** - Bootstrap and foundations (Week 1-2)
- [x] **Phase 2** - ECS, scenes, serialization (Week 3)
- [x] **Phase 3** - Renderer core and materials (Week 4-5)
- [/] **Phase 4** - Asset pipeline and cooker (Week 6)
- [ ] **Phase 5** - Level system and PIE (Week 7)
- [ ] **Phase 6** - Game coding system (Week 8)
- [ ] **Phase 7** - Packaging system (Week 9)
- [ ] **Phase 8** - Physics and navigation (Week 10)
- [ ] **Phase 9** - Lighting and polish (Week 11-12)

See [TODO_LIST.md](TODO_LIST.md) for complete roadmap.

---

## Troubleshooting

### vcpkg not found

Ensure `VCPKG_ROOT` environment variable is set:

```powershell
$env:VCPKG_ROOT
```

### Qt5 not found

Vcpkg should install Qt5 automatically. If not:

```powershell
.\vcpkg install qt5-base:x64-windows
```

### D3D11 Debug Layer errors

Install Windows SDK Graphics Tools from Windows Settings → Apps → Optional Features

### Build errors

- Ensure MSVC compiler is installed (Build Tools for VS 2022)
- Check CMake version: `cmake --version` (need 3.21+)
- In CLion: File → Reload CMake Project
- Clean and reconfigure: `Remove-Item -Recurse Build` then `cmake --preset Debug`

---

## Contributing

This project follows the TODO_LIST.md roadmap strictly. Phase 1 is complete. Next contributions should focus on Phase 2 (ECS and serialization).

---

## License

[Your License Here]

---

## Contact

[Your Contact Info]

---

**Last Updated:** Input System and Lua Documentation - 2026-02-05

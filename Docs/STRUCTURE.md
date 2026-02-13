# Project Structure

Horse Engine follows a modular architecture designed for high performance and strict separation of concerns.

## 📁 Directory Overview

```
Horse-Engine/
├── Engine/                # Core engine modules
│   ├── Runtime/           # High-performance runtime (Zero Qt dependencies)
│   ├── RenderD3D11/       # Direct3D 11 rendering backend
│   ├── Scripting/         # LuaJIT and C++ API bridge
│   └── Physics/           # Jolt Physics integration
├── Editor/                # professional Qt5-based development tool
├── Tools/                 # Offline development utilities
│   ├── Cooker/            # Asset pipeline tool (JSON -> Binary)
│   └── Packager/          # executable and PAK builder
├── Docs/                  # Detailed documentation and schemas
├── ThirdParty/            # Vendor libraries (managed via vcpkg)
└── Projects/              # User project files and assets
```

## 🏗️ Core Modules

### 1. Engine/Runtime

The heart of the engine. It contains the logic for memory management, the job system, time tracking, and the core Win32 platform layer. It is built to be extremely lean and fast.

- **`Core/`**: Memory allocators (Linear, Frame), Job System (Work-stealing thread pool), Logging (spdlog).
- **`Platform/`**: Native Windows windowing and message loops.
- **`Asset/`**: Base classes for GUID-based asset management.

### 2. Engine/RenderD3D11

A dedicated rendering module that handles all communication with the GPU via Direct3D 11.

- **Renderer**: Manages the SwapChain, DeviceContext, and RenderTargets.
- **Shaders**: Handles runtime compilation and management of HLSL shaders.
- **Resources**: High-performance upload queues for textures and meshes.

### 3. Engine/Scripting

Provides the bridge between engine internals and gameplay logic.

- **LuaJIT**: Used for high-speed scripting.
- **sol2**: A modern C++ wrapper for Lua.
- **Native API**: A stable C-style API for high-performance gameplay DLLs.

### 4. Editor

A professional-grade interface built with Qt5. It hosts the Engine's rendering surface and provides tools for scene editing, asset management, and live debugging.

- **Panels**: Dockable widgets for Hierarchy, Inspector, Content Browser, and Console.

---

**Last Updated**: 2026-02-08

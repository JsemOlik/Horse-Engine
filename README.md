# Horse Engine

A high-performance 3D game engine built with **C++20**, **Direct3D 11**, and **Qt5**.

![Horse Engine Logo](https://placehold.co/600x200/2c3e50/ecf0f1?text=Horse+Engine)

## 🚀 Overview

Horse Engine is a modern, modular game engine designed with a strict separation between its high-performance runtime and its feature-rich editor. It leverages modern C++ standards and hardware acceleration to provide a robust platform for 3D application development.

### Key Pillars

- **Performance First**: Cache-friendly data structures and minimal per-frame allocations.
- **Clean Architecture**: Zero Qt dependencies in the runtime; Editor is a separate, professional tool.
- **Modern Standards**: Built with C++20 and Direct3D 11.
- **Developer Friendly**: Comprehensive documentation, Lua scripting, and powerful offline tools.

---

## 🛠️ Getting Started

### Prerequisites

- **Windows 10/11 (x64)**
- **Visual Studio 2022** (with C++ Desktop development workload)
- **CMake 3.21+**
- **vcpkg** (for dependency management)

### Setup & Build

1. **Clone the Repo**:
   ```powershell
   git clone https://github.com/JsemOlik/Horse-Engine.git
   cd Horse-Engine
   ```
2. **Configure with CMake Presets**:
   ```powershell
   cmake --preset Debug
   ```
3. **Build All Targets**:
   ```powershell
   cmake --build Build/Debug
   ```
4. **Run the Editor**:
   ```powershell
   .\Build\Debug\Editor\HorseEditor.exe
   ```

---

## 📚 Documentation

Explore the in-depth documentation to master the Horse Engine:

- 🏗️ **[Project Structure](Docs/STRUCTURE.md)**: Understand the "why" and "where" of the codebase.
- ✨ **[Engine Features](Docs/FEATURES.md)**: Explore the ECS, Rendering pipeline, and Physics system.
- 📜 **[Lua Scripting Reference](Docs/SCRIPTING.md)**: Learn how to write gameplay logic with Lua.
- 🍳 **[Asset Cooker Guide](Docs/COOKER.md)**: How to convert assets for optimized runtime use.
- 📦 **[Game Packager Guide](Docs/PACKAGER.md)**: Distribute your game with a single click.

---

**Last Updated**: 2026-02-08

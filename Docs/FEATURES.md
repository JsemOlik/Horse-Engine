# Engine Features

Horse Engine provides a suite of professional features designed for high-performance 3D development.

## 🧊 Entity Component System (ECS)

Powered by **EnTT**, our ECS is designed for cache efficiency and massive entity counts.

- **Components**: Transform, MeshRenderer, Camera, Light, Script, and Physics.
- **Hierarchy**: Opt-in scene graph with dirty-flag propagation.
- **UUIDs**: Stable identification for every entity and asset in the project.

## 🎨 Rendering Pipeline

A robust Direct3D 11 renderer focused on stability and modern feature support.

- **PBR Materials**: Physically Correct shading (Metallic/Roughness).
- **Shader Permutations**: Dynamic shader generation based on material keywords.
- **Culling**: SIMD-accelerated frustum culling to minimize draw calls.
- **Debug Views**: Built-in wireframe, normal, and depth visualizations.

## 📜 Scripting System

Two ways to build your game:

- **Lua Scripting**: Rapid iteration with hot-reloading support. Full access to the Engine API.
- **C++ Gameplay DLL**: For performance-critical systems. Uses a stable ABI to link with the runtime.

## ⚡ Physics Engine

Integrated with **Jolt Physics** for state-of-the-art simulation.

- **Rigid Bodies**: Dynamic, Static, and Kinematic support.
- **Colliders**: Box, Sphere, Capsule, and Mesh colliders.
- **Queries**: Raycasting, sweeps, and overlap checks.

## 🛠️ Offline Tools

- **Asset Cooker**: Converts source assets (GLTF, PNG, JSON) into optimized binary blobs.
- **Game Packager**: Builds a standalone distribution including the EXE, PAK files, and necessary DLLs.

## 🖥️ Professional Editor

- **Live Viewport**: 60 FPS real-time rendering of your scene.
- **Inspector**: Direct manipulation of component properties.
- **PIE (Play-In-Editor)**: Test your game instantly without leaving the tool.

---

**Last Updated**: 2026-02-08

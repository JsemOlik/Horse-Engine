# Engine Runtime - Core engine without UI

# Find required packages
find_package(nlohmann_json CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

# LuaJIT/Lua finding for vcpkg
find_path(LUA_INCLUDE_DIR NAMES lua.h PATH_SUFFIXES luajit lua5.1 lua)
find_library(LUA_LIBRARY NAMES lua51 luajit lua)

# sol2 finding for vcpkg (it's header-only)
find_path(SOL2_INCLUDE_DIR NAMES sol/sol.hpp)

if(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARY OR NOT SOL2_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find LuaJIT/Lua or sol2!\nLUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}\nLUA_LIBRARY: ${LUA_LIBRARY}\nSOL2_INCLUDE_DIR: ${SOL2_INCLUDE_DIR}")
endif()

add_library(HorseRuntime SHARED
    Source/Platform/Win32Window.cpp
    Source/Platform/Win32Input.cpp
    Source/Core/Time.cpp
    Source/Core/Logging.cpp
    Source/Core/Memory.cpp
    Source/Core/JobSystem.cpp
    Source/Scene/UUID.cpp
    Source/Scene/Entity.cpp
    Source/Scene/Scene.cpp
    Source/Scene/SceneSerializer.cpp
    Source/Project/ProjectSerializer.cpp
    Source/Engine.cpp
    Source/Material.cpp
    Source/MaterialSerializer.cpp
    Source/MaterialRegistry.cpp
    Source/Asset/AssetManager.cpp
    Source/Asset/TextureImporter.cpp
    Source/Asset/MeshImporter.cpp
    Source/Scripting/LuaScriptEngine.cpp
)

target_include_directories(HorseRuntime
    PUBLIC Include
    PRIVATE Source
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(HorseRuntime
    PUBLIC
        spdlog::spdlog
        fmt::fmt
        EnTT::EnTT
        nlohmann_json::nlohmann_json
        glm::glm
        ${LUA_LIBRARY}
        sol2
        d3d11.lib
        dxgi.lib
        dxguid.lib
        d3dcompiler.lib
)

target_compile_definitions(HorseRuntime
    PUBLIC
        HORSE_PLATFORM_WINDOWS=1
    PRIVATE
        HORSE_BUILD_DLL=1
)

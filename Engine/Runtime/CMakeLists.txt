# Engine Runtime - Core engine without UI

# Find required packages
find_package(nlohmann_json CONFIG REQUIRED)
add_library(HorseRuntime STATIC
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
)

target_include_directories(HorseRuntime
    PUBLIC Include
    PRIVATE Source
)

target_link_libraries(HorseRuntime
    PUBLIC
        spdlog::spdlog
        fmt::fmt
        EnTT::EnTT
        nlohmann_json::nlohmann_json
        d3d11.lib
        dxgi.lib
        dxguid.lib
        d3dcompiler.lib
)

target_compile_definitions(HorseRuntime
    PUBLIC
        HORSE_PLATFORM_WINDOWS=1
)

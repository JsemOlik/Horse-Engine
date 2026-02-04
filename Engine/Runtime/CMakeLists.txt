# Engine Runtime - Core engine without UI
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
    Source/Engine.cpp
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
    PRIVATE
        d3d11.lib
        dxgi.lib
        dxguid.lib
)

target_compile_definitions(HorseRuntime
    PUBLIC
        HORSE_PLATFORM_WINDOWS=1
)

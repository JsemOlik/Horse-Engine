project(HorseCooker)

add_executable(HorseCooker
    Source/Main.cpp
    Source/TextureCooker.cpp
    Source/MeshCooker.cpp
    Source/MaterialCooker.cpp
    Source/LevelCooker.cpp
    Source/ProjectCooker.cpp
)

target_link_libraries(HorseCooker
    PRIVATE
        HorseRuntime
        spdlog::spdlog
        fmt::fmt
)

target_include_directories(HorseCooker
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/Engine/Runtime/Include
        ${CMAKE_SOURCE_DIR}/Build/Debug/vcpkg_installed/x64-windows/include
)

set_target_properties(HorseCooker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Tools"
    UNITY_BUILD OFF
)

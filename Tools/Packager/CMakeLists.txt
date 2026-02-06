project(HorsePackager)

add_executable(HorsePackager
    Source/Main.cpp
    Source/PakWriter.cpp
)

find_package(ZLIB REQUIRED)
find_package(physfs CONFIG REQUIRED)

target_link_libraries(HorsePackager
    PRIVATE
    fmt::fmt
    spdlog::spdlog
    ZLIB::ZLIB
    PhysFS::PhysFS
)

target_include_directories(HorsePackager
    PRIVATE
    Source
    ${CMAKE_SOURCE_DIR}/Engine/Runtime/Include
    ${CMAKE_SOURCE_DIR}/Build/Debug/vcpkg_installed/x64-windows/include
)

set_target_properties(HorsePackager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Tools"
    UNITY_BUILD OFF
)

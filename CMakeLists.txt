cmake_minimum_required(VERSION 3.21)
project(HorseEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Editor)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Editor)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Editor)

# Options
option(HORSE_UNITY_BUILD "Enable unity build for faster compilation" ON)
option(HORSE_ENABLE_LTO "Enable Link-Time Optimization" OFF)
option(HORSE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# MSVC-specific flags
if(MSVC)
    add_compile_options(
        /W4                    # Warning level 4
        /permissive-           # Standards conformance
        /EHsc                  # Exception handling
        /wd4251                # Disable DLL interface warnings
        /wd4275                # Disable DLL base class warnings
    )
    
    # Configuration-specific optimizations
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/Zi>
        $<$<CONFIG:Release>:/O2>
    )
    
    # Temporarily disabled due to fmt library constexpr issues with MSVC 19.50
    # if(HORSE_WARNINGS_AS_ERRORS)
    #     add_compile_options(/WX)
    # endif()
    
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Unity build
if(HORSE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# LTO
if(HORSE_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported)
    if(lto_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

# Find packages
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
add_subdirectory(ThirdParty/glm)

# Subdirectories
add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/RenderD3D11)
add_subdirectory(Editor)
add_subdirectory(Tools)
add_subdirectory(Game)

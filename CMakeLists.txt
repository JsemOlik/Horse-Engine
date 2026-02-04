cmake_minimum_required(VERSION 3.21)
project(HorseEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(HORSE_UNITY_BUILD "Enable unity build for faster compilation" ON)
option(HORSE_ENABLE_LTO "Enable Link-Time Optimization" OFF)
option(HORSE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# MSVC-specific flags
if(MSVC)
    add_compile_options(
        /W4                    # Warning level 4
        /permissive-           # Standards conformance
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:DevRelease>:/O2 /Zi>
        $<$<CONFIG:Shipping>:/O2>
    )
    
    if(HORSE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
    
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

# Subdirectories
add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/RenderD3D11)
add_subdirectory(Editor)

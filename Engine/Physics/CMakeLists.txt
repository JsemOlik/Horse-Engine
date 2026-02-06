# Engine/Physics - Jolt Integration

find_package(Jolt CONFIG REQUIRED)

add_library(HorsePhysics STATIC
    Source/PhysicsSystem.cpp
)

target_include_directories(HorsePhysics
    PUBLIC Include
    PRIVATE Source
)

target_link_libraries(HorsePhysics
    PUBLIC
        Jolt::Jolt
        spdlog::spdlog # For logging
        glm::glm       # For math
)

target_compile_definitions(HorsePhysics
    PUBLIC
        JPH_PROFILE_ENABLED
        JPH_DEBUG_Renderer
        HORSE_PLATFORM_WINDOWS=1
    # JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
)

# If utilizing Runtime headers (Core, logging, etc)
# We might need to include Runtime directories, but careful of circular linking.
# For now, we assume Physics relies on standard libraries and potentially some Core headers if we can separate them.
# The Runtime links Physics, so Physics shouldn't link Runtime. 
# However, for Logging macros (HORSE_LOG...), we need Core headers. 
# We will treat Core headers as header-only for the Physics module if possible, or just include the path.
target_include_directories(HorsePhysics PRIVATE "${CMAKE_SOURCE_DIR}/Engine/Runtime/Include")

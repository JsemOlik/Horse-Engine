#pragma once

#define HORSE_API __declspec(dllexport)

// Platform detection
#if !defined(HORSE_PLATFORM_WINDOWS)
    #error "Only Windows is supported"
#endif

// Common types
#include <cstdint>
#include <cstddef>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

namespace Horse {

// Forward declarations
class Engine;
class Window;

} // namespace Horse

#pragma once

#include "HorseEngine/Core.h"
#include <memory>

namespace Horse {

// Linear allocator for sequential allocations (fast, no individual frees)
class LinearAllocator {
public:
    explicit LinearAllocator(size_t capacity);
    ~LinearAllocator();
    
    void* Allocate(size_t size, size_t alignment = 16);
    void Reset();
    
    size_t GetUsed() const { return m_Offset; }
    size_t GetCapacity() const { return m_Capacity; }
    
private:
    u8* m_Buffer = nullptr;
    size_t m_Capacity = 0;
    size_t m_Offset = 0;
};

// Frame allocator - resets every frame for transient allocations
class FrameAllocator {
public:
    static void Initialize(size_t capacity = 16 * 1024 * 1024); // 16 MB default
    static void Shutdown();
    static void Reset(); // Call at frame start
    
    static void* Allocate(size_t size, size_t alignment = 16);
    
private:
    static std::unique_ptr<LinearAllocator> s_Allocator;
};

} // namespace Horse

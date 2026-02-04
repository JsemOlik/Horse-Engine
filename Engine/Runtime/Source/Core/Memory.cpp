#include "HorseEngine/Core/Memory.h"
#include <cstdlib>
#include <cstring>

namespace Horse {

LinearAllocator::LinearAllocator(size_t capacity)
    : m_Capacity(capacity) {
    m_Buffer = static_cast<u8*>(std::malloc(capacity));
    std::memset(m_Buffer, 0, capacity);
}

LinearAllocator::~LinearAllocator() {
    if (m_Buffer) {
        std::free(m_Buffer);
        m_Buffer = nullptr;
    }
}

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t padding = 0;
    size_t currentAddress = reinterpret_cast<size_t>(m_Buffer + m_Offset);
    
    if (alignment > 0 && m_Offset % alignment != 0) {
        padding = alignment - (currentAddress % alignment);
    }
    
    if (m_Offset + padding + size > m_Capacity) {
        return nullptr; // Out of memory
    }
    
    m_Offset += padding;
    void* ptr = m_Buffer + m_Offset;
    m_Offset += size;
    
    return ptr;
}

void LinearAllocator::Reset() {
    m_Offset = 0;
}

// FrameAllocator static members
std::unique_ptr<LinearAllocator> FrameAllocator::s_Allocator;

void FrameAllocator::Initialize(size_t capacity) {
    s_Allocator = std::make_unique<LinearAllocator>(capacity);
}

void FrameAllocator::Shutdown() {
    s_Allocator.reset();
}

void FrameAllocator::Reset() {
    if (s_Allocator) {
        s_Allocator->Reset();
    }
}

void* FrameAllocator::Allocate(size_t size, size_t alignment) {
    return s_Allocator ? s_Allocator->Allocate(size, alignment) : nullptr;
}

} // namespace Horse

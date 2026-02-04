#include "HorseEngine/Core/Time.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Horse {

f64 Time::s_StartTime = 0.0;
f64 Time::s_LastFrameTime = 0.0;
f32 Time::s_DeltaTime = 0.0f;
f32 Time::s_FPS = 0.0f;
i32 Time::s_FrameCount = 0;
f64 Time::s_FPSUpdateTimer = 0.0;

static f64 GetPerformanceFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return static_cast<f64>(frequency.QuadPart);
}

static f64 GetPerformanceCounter() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<f64>(counter.QuadPart) / GetPerformanceFrequency();
}

void Time::Initialize() {
    s_StartTime = GetPerformanceCounter();
    s_LastFrameTime = s_StartTime;
    s_DeltaTime = 0.0f;
    s_FPS = 0.0f;
    s_FrameCount = 0;
    s_FPSUpdateTimer = 0.0;
}

f64 Time::GetTime() {
    return GetPerformanceCounter() - s_StartTime;
}

void Time::Update() {
    f64 currentTime = GetTime();
    s_DeltaTime = static_cast<f32>(currentTime - s_LastFrameTime);
    s_LastFrameTime = currentTime;
    
    s_FrameCount++;
    s_FPSUpdateTimer += s_DeltaTime;
    
    if (s_FPSUpdateTimer >= 1.0) {
        s_FPS = static_cast<f32>(s_FrameCount) / static_cast<f32>(s_FPSUpdateTimer);
        s_FrameCount = 0;
        s_FPSUpdateTimer = 0.0;
    }
}

} // namespace Horse

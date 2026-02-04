#pragma once

#include "HorseEngine/Core.h"

namespace Horse {

class Time {
public:
    static void Initialize();
    
    // Get time since app start in seconds
    static f64 GetTime();
    
    // Get delta time for current frame
    static f32 GetDeltaTime() { return s_DeltaTime; }
    
    // Get FPS
    static f32 GetFPS() { return s_FPS; }
    
    // Update - call once per frame
    static void Update();
    
private:
    static f64 s_StartTime;
    static f64 s_LastFrameTime;
    static f32 s_DeltaTime;
    static f32 s_FPS;
    static i32 s_FrameCount;
    static f64 s_FPSUpdateTimer;
};

} // namespace Horse

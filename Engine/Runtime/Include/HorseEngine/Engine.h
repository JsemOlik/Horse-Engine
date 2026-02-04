#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Platform/Win32Window.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Core/Memory.h"
#include "HorseEngine/Core/JobSystem.h"
#include <memory>

namespace Horse {

class Engine {
public:
    Engine();
    ~Engine();
    
    bool Initialize();
    void Shutdown();
    
    void Run();
    void RunFrame();
    
    Window* GetWindow() const { return m_Window.get(); }
    
    static Engine* Get() { return s_Instance; }
    
private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    
    static Engine* s_Instance;
};

} // namespace Horse

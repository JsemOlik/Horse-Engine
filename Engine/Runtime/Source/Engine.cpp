#include "HorseEngine/Engine.h"

namespace Horse {

Engine* Engine::s_Instance = nullptr;

Engine::Engine() {
    s_Instance = this;
}

Engine::~Engine() {
    s_Instance = nullptr;
}

bool Engine::Initialize() {
    Logger::Initialize();
    HORSE_LOG_CORE_INFO("Horse Engine v0.1.0");
    HORSE_LOG_CORE_INFO("Initializing...");
    
    Time::Initialize();
    FrameAllocator::Initialize();
    JobSystem::Initialize();
    
    HORSE_LOG_CORE_INFO("Job System: {} worker threads", JobSystem::GetThreadCount());
    
    WindowDesc windowDesc;
    windowDesc.Title = L"Horse Engine";
    windowDesc.Width = 1280;
    windowDesc.Height = 720;
    
    m_Window = std::make_unique<Window>(windowDesc);
    
    HORSE_LOG_CORE_INFO("Engine initialized successfully");
    return true;
}

void Engine::Shutdown() {
    HORSE_LOG_CORE_INFO("Shutting down...");
    
    m_Window.reset();
    
    JobSystem::Shutdown();
    FrameAllocator::Shutdown();
    
    HORSE_LOG_CORE_INFO("Engine shutdown complete");
    Logger::Shutdown();
}

void Engine::Run() {
    while (m_Running && m_Window->PollEvents()) {
        RunFrame();
    }
}

void Engine::RunFrame() {
    FrameAllocator::Reset();
    Time::Update();
    
    // Main loop work here
}

} // namespace Horse

#include "HorseEngine/Engine.h"
#include "HorseEngine/Core/Memory.h"
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Scripting/LuaScriptEngine.h"
#include <filesystem>
#include <iostream>

namespace Horse {

Engine *Engine::s_Instance = nullptr;

Engine::Engine() { s_Instance = this; }

Engine::~Engine() { s_Instance = nullptr; }

bool Engine::Initialize(bool headless) {
  Logger::Initialize();
  HORSE_LOG_CORE_INFO("Horse Engine v0.1.0");
  HORSE_LOG_CORE_INFO("Initializing...");

  Time::Initialize();
  FrameAllocator::Initialize();
  JobSystem::Initialize();

  HORSE_LOG_CORE_INFO("Job System: {} worker threads",
                      JobSystem::GetThreadCount());

  if (!headless) {
    WindowDesc windowDesc;
    windowDesc.Title = L"Horse Engine";
    windowDesc.Width = 1280;
    windowDesc.Height = 720;

    m_Window = std::make_unique<Window>(windowDesc);
    HORSE_LOG_CORE_INFO("Window initialized");
  } else {
    HORSE_LOG_CORE_INFO("Running in headless mode (no window)");
  }

  HORSE_LOG_CORE_INFO("Engine initialized successfully");

  LuaScriptEngine::Init();

  LoadGameDLL("HorseGame.dll");

  return true;
}

void Engine::Shutdown() {
  HORSE_LOG_CORE_INFO("Shutting down...");

  LuaScriptEngine::Shutdown();

  UnloadGameDLL();

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
  if (m_GameModule) {
    m_GameModule->OnUpdate(Time::GetDeltaTime());
  }
}

void Engine::LoadGameDLL(const std::string &dllPath) {
  HORSE_LOG_CORE_INFO("Loading Game DLL: {}", dllPath);

  m_LoadedDLLPath = dllPath;

  if (!std::filesystem::exists(dllPath)) {
    HORSE_LOG_CORE_ERROR("Game DLL not found: {}", dllPath);
    return;
  }

  // Copy DLL to temp file to avoid locking the original
  std::string tempPath = dllPath + ".temp";
  try {
    std::filesystem::copy_file(
        dllPath, tempPath, std::filesystem::copy_options::overwrite_existing);
  } catch (std::filesystem::filesystem_error &e) {
    HORSE_LOG_CORE_ERROR("Failed to copy Game DLL: {}", e.what());
    // Fallback? or return?
    // Let's try loading original if copy fails? No, better fail.
    return;
  }

  m_GameDLL = LoadLibraryA(tempPath.c_str());
  if (!m_GameDLL) {
    HORSE_LOG_CORE_ERROR("Failed to load Game DLL: {}", tempPath);
    return;
  }

  CreateGameModuleFunc createFunc =
      (CreateGameModuleFunc)GetProcAddress(m_GameDLL, "CreateGameModule");
  if (!createFunc) {
    HORSE_LOG_CORE_ERROR("Failed to find CreateGameModule symbol in Game DLL");
    FreeLibrary(m_GameDLL);
    m_GameDLL = nullptr;
    return;
  }

  m_GameModule = createFunc();
  if (m_GameModule) {
    m_GameModule->OnLoad();
    HORSE_LOG_CORE_INFO("Game Module loaded successfully");
  } else {
    HORSE_LOG_CORE_ERROR("Failed to create Game Module");
  }
}

void Engine::ReloadGameDLL() {
  if (m_LoadedDLLPath.empty()) {
    HORSE_LOG_CORE_WARN("Cannot reload Game DLL: No DLL loaded previously.");
    return;
  }

  HORSE_LOG_CORE_INFO("Reloading Game Module...");
  UnloadGameDLL();
  LoadGameDLL(m_LoadedDLLPath);
}

void Engine::UnloadGameDLL() {
  if (m_GameModule) {
    m_GameModule->OnShutdown();
    delete m_GameModule;
    m_GameModule = nullptr;
  }

  if (m_GameDLL) {
    FreeLibrary(m_GameDLL);
    m_GameDLL = nullptr;
    HORSE_LOG_CORE_INFO("Game DLL unloaded");
  }
}

GameModule *Engine::GetGameModule() const { return m_GameModule; }

} // namespace Horse

#include "HorseEngine/Engine.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Core/Memory.h"
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Project/Project.h"
#include "HorseEngine/Render/Renderer.h"
#include "HorseEngine/Scripting/LuaScriptEngine.h"
#include <filesystem>
#include <iostream>
#include <vector>

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
    windowDesc.VSync = false;

    m_Window = std::make_unique<Window>(windowDesc);
    HORSE_LOG_CORE_INFO("Window initialized");
  } else {
    HORSE_LOG_CORE_INFO("Running in headless mode (no window)");
  }

  HORSE_LOG_CORE_INFO("Engine initialized successfully");

  LuaScriptEngine::Init();

  // Default Input Mappings
  Input::RegisterAction("Jump", {KEY_SPACE});
  Input::RegisterAxis("Forward", {KEY_W}, {KEY_S});
  Input::RegisterAxis("Right", {KEY_D}, {KEY_A});

  // Standalone mode: Load project binary if it exists
  std::filesystem::path projectBin = "Game.project.bin";
  HORSE_LOG_CORE_INFO("Checking for project binary via FileSystem...");
  if (FileSystem::Exists(projectBin)) {
    auto project = Project::LoadFromBinary(projectBin);
    if (project) {
      Project::SetActive(project);
      HORSE_LOG_CORE_INFO(
          "Loaded Project configuration from binary. DefaultLevelGUID: {}",
          project->GetConfig().DefaultLevelGUID);
    } else {
      HORSE_LOG_CORE_ERROR("Failed to load Project configuration from binary!");
    }
  } else {
    HORSE_LOG_CORE_WARN("Game.project.bin not found via FileSystem");
  }

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

  // Update
  if (m_GameModule) {
    m_GameModule->OnUpdate(Time::GetDeltaTime());
  }

  // Render
  if (m_Renderer && m_GameModule) {
    m_Renderer->BeginFrame();
    m_Renderer->Clear(0.1f, 0.1f, 0.1f, 1.0f);

    Scene *scene = m_GameModule->GetActiveScene();
    if (scene) {
      m_Renderer->RenderScene(scene);
    }

    m_Renderer->EndFrame();
    m_Renderer->Present();
  }
}

void Engine::LoadGameDLL(const std::string &dllPath) {
  HORSE_LOG_CORE_INFO("Loading Game DLL: {}", dllPath);

  // Copy DLL to avoid locking the original (hot reloading prep)
  // For now, straight load
  m_GameDLL = LoadLibraryA(dllPath.c_str());
  if (!m_GameDLL) {
    HORSE_LOG_CORE_ERROR("Failed to load Game DLL: {}", dllPath);
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

#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Core/JobSystem.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Memory.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Platform/Win32Window.h"
#include <memory>

namespace Horse {

class GameModule;
class LuaScriptEngine;
class Renderer;

class HORSE_API Engine {
public:
  Engine();
  ~Engine();

  bool Initialize(bool headless = false);
  void Shutdown();

  void LoadGameDLL(const std::string &dllPath);
  void UnloadGameDLL();

  void Run();
  void RunFrame();

  Window *GetWindow() const { return m_Window.get(); }
  GameModule *GetGameModule() const;
  void SetRenderer(Renderer *renderer) { m_Renderer = renderer; }
  Renderer *GetRenderer() const { return m_Renderer; }

  static Engine *Get() { return s_Instance; }

private:
  std::unique_ptr<Window> m_Window;
  Renderer *m_Renderer = nullptr;
  bool m_Running = true;

  GameModule *m_GameModule = nullptr;
  HMODULE m_GameDLL = nullptr;

  static Engine *s_Instance;
};

} // namespace Horse

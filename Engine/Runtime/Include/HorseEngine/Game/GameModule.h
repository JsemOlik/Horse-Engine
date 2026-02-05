#pragma once

#include "HorseEngine/Core.h"

namespace Horse {

class HORSE_API GameModule {
public:
  virtual ~GameModule() = default;

  virtual void OnLoad() = 0;
  virtual void OnShutdown() = 0;

  virtual void OnUpdate(float deltaTime) = 0;
};

// Function pointer type for creating the game module
using CreateGameModuleFunc = GameModule *(*)();

} // namespace Horse

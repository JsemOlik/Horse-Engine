#pragma once

#include "HorseEngine/Scene/Entity.h"
#include <string>
#include <vector>


namespace Horse {

class HORSE_API GameModule {
public:
  virtual ~GameModule() = default;

  virtual void OnLoad() = 0;
  virtual void OnShutdown() = 0;

  virtual void OnUpdate(float deltaTime) = 0;

  virtual std::vector<std::string> GetAvailableScripts() const { return {}; }
  virtual void CreateScript(const std::string &name, Entity entity) {}
};

// Function pointer type for creating the game module
using CreateGameModuleFunc = GameModule *(*)();

} // namespace Horse

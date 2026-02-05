#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/Entity.h"
#include <sol/sol.hpp>
#include <string>

namespace Horse {

class HORSE_API LuaScriptEngine {
public:
  static void Init();
  static void Shutdown();

  static void OnCreateEntity(Entity entity);
  static void OnUpdateEntity(Entity entity, float deltaTime);
  static void OnDestroyEntity(Entity entity);

  static sol::state &GetState() { return *s_LuaState; }

private:
  static void BindEntity();
  static void BindLogging();
  static void BindInput();

private:
  static sol::state *s_LuaState;
  static std::unordered_map<UUID, sol::table> s_ScriptInstances;
};

} // namespace Horse

#include "HorseEngine/Scripting/LuaScriptEngine.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"

namespace Horse {

sol::state *LuaScriptEngine::s_LuaState = nullptr;

void LuaScriptEngine::Init() {
  s_LuaState = new sol::state();
  s_LuaState->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math,
                             sol::lib::table);

  BindLogging();
  BindEntity();

  HORSE_LOG_CORE_INFO("LuaScriptEngine initialized.");
}

void LuaScriptEngine::Shutdown() {
  delete s_LuaState;
  s_LuaState = nullptr;
}

void LuaScriptEngine::BindLogging() {
  auto horse = s_LuaState->create_named_table("Horse");

  horse.set_function("LogInfo", [](const std::string &message) {
    HORSE_LOG_CORE_INFO("[Lua] {}", message);
  });
  horse.set_function("LogWarn", [](const std::string &message) {
    HORSE_LOG_CORE_WARN("[Lua] {}", message);
  });
  horse.set_function("LogError", [](const std::string &message) {
    HORSE_LOG_CORE_ERROR("[Lua] {}", message);
  });
}

void LuaScriptEngine::BindEntity() {
  auto horse = s_LuaState->get<sol::table>("Horse");

  // Vec3 binding for Transform
  horse.new_usertype<glm::vec3>(
      "Vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
      "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z,
      sol::meta_function::addition,
      [](const glm::vec3 &a, const glm::vec3 &b) { return a + b; },
      sol::meta_function::subtraction,
      [](const glm::vec3 &a, const glm::vec3 &b) { return a - b; },
      sol::meta_function::multiplication,
      [](const glm::vec3 &a, float b) { return a * b; });

  // TransformComponent
  horse.new_usertype<TransformComponent>(
      "Transform", "Position",
      sol::property(
          [](TransformComponent &t) {
            return glm::vec3(t.Position[0], t.Position[1], t.Position[2]);
          },
          [](TransformComponent &t, const glm::vec3 &v) {
            t.Position[0] = v.x;
            t.Position[1] = v.y;
            t.Position[2] = v.z;
          }),
      "Rotation",
      sol::property(
          [](TransformComponent &t) {
            return glm::vec3(t.Rotation[0], t.Rotation[1], t.Rotation[2]);
          },
          [](TransformComponent &t, const glm::vec3 &v) {
            t.Rotation[0] = v.x;
            t.Rotation[1] = v.y;
            t.Rotation[2] = v.z;
          }),
      "Scale",
      sol::property(
          [](TransformComponent &t) {
            return glm::vec3(t.Scale[0], t.Scale[1], t.Scale[2]);
          },
          [](TransformComponent &t, const glm::vec3 &v) {
            t.Scale[0] = v.x;
            t.Scale[1] = v.y;
            t.Scale[2] = v.z;
          }));

  // Entity
  horse.new_usertype<Entity>(
      "Entity", "HasTransform", &Entity::HasComponent<TransformComponent>,
      "GetTransform", &Entity::GetComponent<TransformComponent>, "GetName",
      [](Entity &e) { return e.GetComponent<TagComponent>().Name; });
}

void LuaScriptEngine::OnCreateEntity(Entity entity) {
  if (!entity.HasComponent<ScriptComponent>())
    return;

  auto &sc = entity.GetComponent<ScriptComponent>();
  if (sc.ScriptPath.empty())
    return;

  auto result =
      s_LuaState->script_file(sc.ScriptPath, sol::script_pass_on_error);
  if (!result.valid()) {
    sol::error err = result;
    HORSE_LOG_CORE_ERROR("Failed to load Lua script {}: {}", sc.ScriptPath,
                         err.what());
    return;
  }

  sol::table self = result;
  if (self["OnCreate"].valid()) {
    self["OnCreate"](self, entity);
  }

  // Store the table in the component for future calls?
  // We'll need a way to keep the instance alive.
  // For now let's just use globals or re-run, but that's not ideal.
  // Let's assume the script returns a table representing the "script instance".
}

void LuaScriptEngine::OnUpdateEntity(Entity entity, float deltaTime) {
  if (!entity.HasComponent<ScriptComponent>())
    return;

  auto &sc = entity.GetComponent<ScriptComponent>();
  if (sc.ScriptPath.empty())
    return;

  // This is very slow (re-parsing every frame), but let's get it working first.
  // We'll optimize with a cache mapping entities to sol::table instances.
  auto result =
      s_LuaState->script_file(sc.ScriptPath, sol::script_pass_on_error);
  if (result.valid()) {
    sol::table self = result;
    if (self["OnUpdate"].valid()) {
      self["OnUpdate"](self, entity, deltaTime);
    }
  }
}

} // namespace Horse

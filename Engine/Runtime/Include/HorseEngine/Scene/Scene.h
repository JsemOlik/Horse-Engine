#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/UUID.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace Horse {

enum class SceneState { Edit = 0, Play, Pause, Loading };
enum class LoadingStage { None = 0, Assets, Components, Scripts, Ready };

class Scene {
public:
  Scene(const std::string &name = "Untitled Scene");
  ~Scene();

  static std::shared_ptr<Scene> Copy(const std::shared_ptr<Scene> &other);

  void OnRuntimeStart();
  void OnRuntimeStop();
  void OnRuntimeUpdate(float deltaTime);

  SceneState GetState() const { return m_State; }
  void SetState(SceneState state) { m_State = state; }

  Entity CreateEntity(const std::string &name = "Entity");
  Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
  void DestroyEntity(Entity entity);

  Entity GetEntityByUUID(UUID uuid);
  Entity GetEntityByName(const std::string &name);

  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }

  entt::registry &GetRegistry() { return m_Registry; }
  const entt::registry &GetRegistry() const { return m_Registry; }

  // Hierarchy management
  void SetEntityParent(Entity child, Entity parent);
  void RemoveParent(Entity child);
  Entity GetParent(Entity entity);

  // Update systems
  void OnUpdate(float deltaTime);

private:
  void UpdateTransformHierarchy();
  void UpdateEntityTransform(Entity entity, const glm::mat4 &parentTransform);
  void UpdateStagedLoad();
  void TriggerAssetLoads();

private:
  std::string m_Name;
  entt::registry m_Registry;
  std::unordered_map<UUID, entt::entity> m_EntityMap;
  SceneState m_State = SceneState::Edit;
  LoadingStage m_LoadingStage = LoadingStage::None;
  std::vector<std::string> m_LoadingQueue;
};

} // namespace Horse

#include "HorseEngine/Scene/Entity.inl"

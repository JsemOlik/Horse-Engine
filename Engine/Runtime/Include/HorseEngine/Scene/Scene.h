#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/UUID.h"
#include <entt/entt.hpp>
#include <string>
#include <unordered_map>

namespace Horse {

class Scene {
public:
  Scene(const std::string &name = "Untitled Scene");
  ~Scene();

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
  void SetParent(Entity child, Entity parent);
  void RemoveParent(Entity child);
  Entity GetParent(Entity entity);

  // Update systems
  void OnUpdate(float deltaTime);

private:
  void UpdateTransformHierarchy();

private:
  std::string m_Name;
  entt::registry m_Registry;
  std::unordered_map<UUID, entt::entity> m_EntityMap;
};

} // namespace Horse

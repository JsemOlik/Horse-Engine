#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/UUID.h"
#include <entt/entt.hpp>

namespace Horse {

class Scene;

class HORSE_API Entity {
public:
  Entity() = default;
  Entity(entt::entity handle, Scene *scene);

  template <typename T, typename... Args> T &AddComponent(Args &&...args);

  template <typename T> T &GetComponent();

  template <typename T> bool HasComponent();

  template <typename T> void RemoveComponent();

  operator bool() const {
    return m_EntityHandle != entt::null && m_Scene != nullptr;
  }
  operator entt::entity() const { return m_EntityHandle; }
  operator u32() const { return static_cast<u32>(m_EntityHandle); }

  bool operator==(const Entity &other) const {
    return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
  }

  bool operator!=(const Entity &other) const { return !(*this == other); }

  entt::entity GetHandle() const { return m_EntityHandle; }
  Scene *GetScene() const { return m_Scene; }
  UUID GetUUID();

private:
  entt::entity m_EntityHandle = entt::null;
  Scene *m_Scene = nullptr;
};

} // namespace Horse

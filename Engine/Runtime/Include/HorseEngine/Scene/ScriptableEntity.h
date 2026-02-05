#pragma once

#include "HorseEngine/Scene/Entity.h"

namespace Horse {

class ScriptableEntity {
public:
  virtual ~ScriptableEntity() = default;

  template <typename T> T &GetComponent() { return m_Entity.GetComponent<T>(); }

protected:
  virtual void OnCreate() {}
  virtual void OnDestroy() {}
  virtual void OnUpdate(float deltaTime) {}

private:
  Entity m_Entity;
  friend class Scene;
};

} // namespace Horse

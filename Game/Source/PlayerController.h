#pragma once

#include "HorseEngine/Physics/PhysicsComponents.h"
#include "HorseEngine/Scene/ScriptableEntity.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>


namespace Horse {

class PlayerController : public ScriptableEntity {
public:
  void OnCreate() override;
  void OnUpdate(float deltaTime) override;
  void OnDestroy() override;

private:
  float m_Speed = 5.0f;
  float m_JumpForce = 500.0f; // Adjusted for Jolt units (forces are robust)
  float m_MouseSensitivity = 0.1f;

  RigidBodyComponent *m_RigidBody = nullptr;
  JPH::BodyInterface *m_BodyInterface = nullptr;
};

} // namespace Horse

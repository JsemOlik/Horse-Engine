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
  float m_JumpForce = 5.0f;

  // Look Rotation State
  float m_Pitch = 0.0f; // Camera explicit pitch
  float m_Yaw = 0.0f;   // Body explicit yaw
  float m_Sensitivity = 0.1f;
  float m_LastMouseX = 0.0f;
  float m_LastMouseY = 0.0f;
  bool m_FirstMouse = true;

  RigidBodyComponent *m_RigidBody = nullptr;
  JPH::BodyInterface *m_BodyInterface = nullptr;

  Horse::Entity m_CameraEntity; // Handle to child camera
};

} // namespace Horse

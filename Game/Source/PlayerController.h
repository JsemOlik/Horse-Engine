#pragma once

#include "HorseEngine/Physics/PhysicsComponents.h"
#include "HorseEngine/Scene/ScriptableEntity.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <glm/glm.hpp>

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
  bool m_OldJumpPressed = false;

  // Quake Movement Constants (Units = Inches)
  float m_Gravity = 800.0f;
  float m_MaxSpeed = 320.0f;
  float m_Accelerate = 10.0f;
  float m_AirAccelerate = 0.7f;
  float m_Friction = 6.0f;
  float m_StopSpeed = 100.0f;
  float m_JumpImpulse = 270.0f; // Quake jump speed

  // Helpers
  void ApplyFriction(glm::vec3 &velocity, float deltaTime);
  void Accelerate(glm::vec3 &velocity, glm::vec3 wishDir, float wishSpeed,
                  float accel, float deltaTime);
  void AirAccelerate(glm::vec3 &velocity, glm::vec3 wishDir, float wishSpeed,
                     float accel, float deltaTime);
  bool IsOnGround();

  // Unit Conversion (1 meter = 39.37 inches)
  const float m_UnitsToMeters = 1.0f / 39.37f;
  const float m_MetersToUnits = 39.37f;

  inline float ToMeters(float units) const { return units * m_UnitsToMeters; }
  inline float ToUnits(float meters) const { return meters * m_MetersToUnits; }
  inline glm::vec3 ToMeters(const glm::vec3 &v) const {
    return v * m_UnitsToMeters;
  }
  inline glm::vec3 ToUnits(const glm::vec3 &v) const {
    return v * m_MetersToUnits;
  }
};

} // namespace Horse

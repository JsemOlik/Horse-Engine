#include "PlayerController.h"
#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Physics/PhysicsComponents.h"
#include "HorseEngine/Physics/PhysicsSystem.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"
#include <glm/gtc/matrix_transform.hpp>

// Jolt Includes
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <algorithm>

namespace Horse {

void PlayerController::OnCreate() {
  if (GetComponent<RigidBodyComponent>().RuntimeBody) {
    m_RigidBody = &GetComponent<RigidBodyComponent>();
    m_RigidBody->LockRotationX = true;
    m_RigidBody->LockRotationZ = true;

    // Access PhysicsSystem via Scene
    auto scene = GetEntity().GetScene();
    if (scene && scene->GetPhysicsSystem()) {
      m_BodyInterface = scene->GetPhysicsSystem()->GetBodyInterface();

      // Lock rotation for a standard character controller feel
      JPH::BodyID bodyID =
          static_cast<JPH::Body *>(m_RigidBody->RuntimeBody)->GetID();
      m_BodyInterface->SetRotation(bodyID, JPH::Quat::sIdentity(),
                                   JPH::EActivation::Activate);
    }
  }

  // Find Child Camera
  if (GetEntity().HasComponent<RelationshipComponent>()) {
    auto &rel = GetComponent<RelationshipComponent>();
    entt::entity childHandle = rel.FirstChild;
    while (childHandle != entt::null) {
      Horse::Entity child(childHandle, GetEntity().GetScene());
      if (child.HasComponent<CameraComponent>()) {
        m_CameraEntity = child;
        break;
      }
      if (child.HasComponent<RelationshipComponent>()) {
        childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
      } else {
        break;
      }
    }
  }

  // Init Mouse State
  auto mousePos = Input::GetMousePosition();
  m_LastMouseX = mousePos.X;
  m_LastMouseY = mousePos.Y;
  m_FirstMouse = true;
}

void PlayerController::OnUpdate(float deltaTime) {
  if (!m_RigidBody || !m_RigidBody->RuntimeBody)
    return;

  JPH::BodyID bodyID =
      static_cast<JPH::Body *>(m_RigidBody->RuntimeBody)->GetID();

  // -------------------------------------------------------------------------
  // Look Rotation
  // -------------------------------------------------------------------------

  // 1. Calculate Mouse Delta
  auto mousePos = Input::GetMousePosition();
  if (m_FirstMouse) {
    m_LastMouseX = mousePos.X;
    m_LastMouseY = mousePos.Y;
    m_FirstMouse = false;
  }

  float xOffset = mousePos.X - m_LastMouseX;
  float yOffset =
      m_LastMouseY -
      mousePos.Y; // Reversed since Y-coords range from bottom to top commonly?
                  // Or top-down? Win32 usually is Top-Left origin.
  // If Win32 is Top-Left (0,0), then Down increases Y. pitch up implies looking
  // "up". Let's assume standard mouse look: moving mouse UP (negative Y delta)
  // should pitch UP. So LastY - CurrentY gives positive when moving Up.
  // Correct.

  m_LastMouseX = mousePos.X;
  m_LastMouseY = mousePos.Y;

  // IMPORTANT: Rotate if the cursor is locked (standard gameplay feel)
  if (Input::GetCursorMode() == CursorMode::Locked) {
    xOffset *= m_Sensitivity;
    yOffset *= m_Sensitivity;

    m_Yaw += xOffset;
    m_Pitch -= yOffset; // Inverted Y-axis fix: Mouse UP (negative dy) should
                        // Increase Pitch (Look Up)

    // Clamp Pitch
    if (m_Pitch > 89.0f)
      m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
      m_Pitch = -89.0f;

    // 2. Rotate Body (Yaw)
    // Physics Rotation - convert Yaw (degrees) to Radians and around Y axis
    // Note: Jolt uses Radians.
    // -m_Yaw because... coordinate systems. Let's try standard first.
    // Convert to radians for Jolt. Standard yaw rotation (Positive = Left, or
    // depends on engine) User reports side-to-side is inverted, so let's use
    // positive m_Yaw.
    JPH::Quat rotation =
        JPH::Quat::sRotation(JPH::Vec3::sAxisY(), glm::radians(m_Yaw));
    m_BodyInterface->SetRotation(bodyID, rotation, JPH::EActivation::Activate);

    // 3. Rotate Camera (Pitch)
    if (m_CameraEntity) {
      auto &transform = m_CameraEntity.GetComponent<TransformComponent>();
      transform.Rotation[0] = m_Pitch;
      // transform.Rotation.y = 0; // Local rotation, so Yaw is handled by
      // parent transform.Rotation.z = 0;
    }
  }

  // -------------------------------------------------------------------------
  // Movement
  // -------------------------------------------------------------------------

  // Current Velocity in Units
  JPH::Vec3 worldVelocity = m_BodyInterface->GetLinearVelocity(bodyID);
  glm::vec3 velocity = ToUnits(glm::vec3(
      worldVelocity.GetX(), worldVelocity.GetY(), worldVelocity.GetZ()));

  bool onGround = IsOnGround();

  // Calculate Forward/Right vectors based on YAW
  // Using engine convention: +Z is Forward, +X is Right
  float yawRad = glm::radians(m_Yaw);
  glm::vec3 forward(sin(yawRad), 0, cos(yawRad));
  glm::vec3 right(cos(yawRad), 0, -sin(yawRad));

  glm::vec3 wishDir(0.0f);
  if (Input::IsKeyPressed(KEY_W))
    wishDir += forward;
  if (Input::IsKeyPressed(KEY_S))
    wishDir -= forward;
  if (Input::IsKeyPressed(KEY_A))
    wishDir -= right;
  if (Input::IsKeyPressed(KEY_D))
    wishDir += right;

  float wishSpeed = glm::length(wishDir);
  if (wishSpeed > 0.0f) {
    wishDir = glm::normalize(wishDir);
    wishSpeed = m_MaxSpeed;
  }

  bool jumpPressed = Input::IsKeyPressed(KEY_SPACE);

  if (onGround) {
    // Ground Move
    ApplyFriction(velocity, deltaTime);
    Accelerate(velocity, wishDir, wishSpeed, m_Accelerate, deltaTime);

    // Jump (Pogo prevention)
    if (jumpPressed && !m_OldJumpPressed) {
      velocity.y = m_JumpImpulse;
    }
  } else {
    // Air Move
    AirAccelerate(velocity, wishDir, wishSpeed, m_AirAccelerate, deltaTime);

    // Apply Gravity
    velocity.y -= m_Gravity * deltaTime;
  }

  m_OldJumpPressed = jumpPressed;

  // Set new world velocity
  glm::vec3 worldVel = ToMeters(velocity);
  m_BodyInterface->SetLinearVelocity(
      bodyID, JPH::Vec3(worldVel.x, worldVel.y, worldVel.z));
  m_BodyInterface->ActivateBody(bodyID);
}

void PlayerController::ApplyFriction(glm::vec3 &velocity, float deltaTime) {
  float speed = glm::length(velocity);
  if (speed < 0.1f)
    return;

  float drop = 0.0f;
  float control = speed < m_StopSpeed ? m_StopSpeed : speed;
  drop += control * m_Friction * deltaTime;

  float newSpeed = speed - drop;
  if (newSpeed < 0.0f)
    newSpeed = 0.0f;
  newSpeed /= speed;

  velocity *= newSpeed;
}

void PlayerController::Accelerate(glm::vec3 &velocity, glm::vec3 wishDir,
                                  float wishSpeed, float accel,
                                  float deltaTime) {
  float currentSpeed = glm::dot(velocity, wishDir);
  float addSpeed = wishSpeed - currentSpeed;
  if (addSpeed <= 0)
    return;

  float accelSpeed = accel * deltaTime * wishSpeed;
  if (accelSpeed > addSpeed)
    accelSpeed = addSpeed;

  velocity += wishDir * accelSpeed;
}

void PlayerController::AirAccelerate(glm::vec3 &velocity, glm::vec3 wishDir,
                                     float wishSpeed, float accel,
                                     float deltaTime) {
  float wishSpd = wishSpeed;
  if (wishSpd > 30.0f)
    wishSpd = 30.0f;

  float currentSpeed = glm::dot(velocity, wishDir);
  float addSpeed = wishSpd - currentSpeed;
  if (addSpeed <= 0)
    return;

  float accelSpeed = accel * wishSpeed * deltaTime;
  if (accelSpeed > addSpeed)
    accelSpeed = addSpeed;

  velocity += wishDir * accelSpeed;
}

bool PlayerController::IsOnGround() {
  auto scene = GetEntity().GetScene();
  if (!scene || !scene->GetPhysicsSystem())
    return false;

  auto &transform = this->template GetComponent<Horse::TransformComponent>();
  // Raycast from center to slightly below feet
  // Assuming cube is 1x2x1 meters (39.37 x 78.74 x 39.37 units)
  // Floor is at y - 1.0m (approx)
  glm::vec3 start = {transform.Position[0], transform.Position[1],
                     transform.Position[2]};
  glm::vec3 end = start - glm::vec3(0, 1.1f, 0); // 1.1m down

  // Use filtered RayCast to ignore ourselves
  auto result =
      scene->GetPhysicsSystem()->RayCast(start, end, m_RigidBody->RuntimeBody);

  // Normal.y > 0.7 means slope is less than ~45 degrees
  return result.Hit && result.Fraction < 1.0f && result.Normal.y > 0.7f;
}

void PlayerController::OnDestroy() {
  // Cleanup if needed
}

} // namespace Horse

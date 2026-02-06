#include "PlayerController.h"
#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Physics/PhysicsSystem.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"

// Jolt Includes
#include <Jolt/Physics/Body/Body.h>

namespace Horse {

void PlayerController::OnCreate() {
  if (GetComponent<RigidBodyComponent>().RuntimeBody) {
    m_RigidBody = &GetComponent<RigidBodyComponent>();

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

  // IMPORTANT: Only rotate if right mouse button is held (Editor Mode style) OR
  // if we are in "Capture Mode" For now, let's require Right Mouse Button for
  // the Player Controller too, to avoid annoyance in Editor.
  if (Input::IsMouseButtonPressed(KEY_RBUTTON)) {
    xOffset *= m_Sensitivity;
    yOffset *= m_Sensitivity;

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    // Clamp Pitch
    if (m_Pitch > 89.0f)
      m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
      m_Pitch = -89.0f;

    // 2. Rotate Body (Yaw)
    // Physics Rotation - convert Yaw (degrees) to Radians and around Y axis
    // Note: Jolt uses Radians.
    // -m_Yaw because... coordinate systems. Let's try standard first.
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
  JPH::Vec3 velocity = m_BodyInterface->GetLinearVelocity(bodyID);
  JPH::Vec3 moveDir = JPH::Vec3::sZero();

  // Calculate Forward/Right vectors based on YAW
  // Simple trig or use the quaternion we just set.
  float yawRad = glm::radians(m_Yaw);
  JPH::Vec3 forward(
      sin(yawRad), 0,
      cos(yawRad)); // Z is forward? In standard OpenGL, -Z is forward.
  // Let's assume +Z is forward for now, fix if inverted.
  // Actually, if we rotate the body, "Forward" in Local space is just (0,0,1).
  // Jolt's linear velocity is in World Space. So we need World Forward.

  // Forward vector from Yaw:
  // x = sin(yaw), z = cos(yaw)

  JPH::Vec3 right(cos(yawRad), 0, -sin(yawRad));

  if (Input::IsKeyPressed(KEY_W))
    moveDir += forward;
  if (Input::IsKeyPressed(KEY_S))
    moveDir -= forward;
  if (Input::IsKeyPressed(KEY_A))
    moveDir -= right;
  if (Input::IsKeyPressed(KEY_D))
    moveDir += right;

  if (moveDir.LengthSq() > 0.0f) {
    moveDir = moveDir.Normalized() * m_Speed;
    // Preserve vertical velocity (gravity)
    moveDir.SetY(velocity.GetY());

    m_BodyInterface->SetLinearVelocity(bodyID, moveDir);
    m_BodyInterface->ActivateBody(bodyID);
  } else {
    // Dampen horizontal velocity if no input?
    // For now just keep gravity.
    m_BodyInterface->SetLinearVelocity(bodyID,
                                       JPH::Vec3(0, velocity.GetY(), 0));
  }

  // -------------------------------------------------------------------------
  // Jump
  // -------------------------------------------------------------------------
  if (Input::IsKeyPressed(KEY_SPACE)) {
    // Simple ground check: if vertical velocity is near zero
    if (abs(velocity.GetY()) < 0.1f) {
      m_BodyInterface->AddImpulse(bodyID, JPH::Vec3(0, m_JumpForce, 0));
    }
  }
}

void PlayerController::OnDestroy() {
  // Cleanup if needed
}

} // namespace Horse

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

      // Lock rotation for a standard character controller feel (so it doesn't
      // tumble) Note: Ideally this should be a setting on RigidBodyComponent,
      // but we can enforce it here for now.
      JPH::BodyID bodyID =
          static_cast<JPH::Body *>(m_RigidBody->RuntimeBody)->GetID();
      m_BodyInterface->SetRotation(bodyID, JPH::Quat::sIdentity(),
                                   JPH::EActivation::Activate);
    }
    // Set max angular velocity to 0 to prevent spinning from collisions?
    // Better way in Jolt:
    // body->GetMotionProperties()->SetInverseInertiaDiagonal(JPH::Vec3::sZero());
    // But we need access to the Body* or lock flags. Jolt has EallowedDOFs.
    // For now, let's just apply forces.
  }
}

void PlayerController::OnUpdate(float deltaTime) {
  if (!m_RigidBody || !m_RigidBody->RuntimeBody)
    return;

  JPH::BodyID bodyID =
      static_cast<JPH::Body *>(m_RigidBody->RuntimeBody)->GetID();

  // -------------------------------------------------------------------------
  // Look Rotation (Standard FPS Camera)
  // -------------------------------------------------------------------------
  // We need to rotate the ENTITY (transform) based on mouse input.
  // However, since physics drives position/rotation, we should rotate the
  // physics body OR rotate the camera child entity separately. Unity style:
  // Player object rotates Y (Yaw), Camera child rotates X (Pitch).

  // Let's assume standard ECS structure: Player Entity has Camera as Child.
  // But here we are the Player Entity.

  // For simplicity in this first pass, let's just do movement relative to World
  // or Local. If we want Yaw rotation on the body:

  static float yaw = 0.0f;
  static float pitch = 0.0f;

  // This is a bit hacky without a proper Input system for "Mouse Delta",
  // but let's assume we can get simple key-based rotation or wait for Mouse
  // Delta API. START-UP HACK: Just movement for now.

  // -------------------------------------------------------------------------
  // Movement
  // -------------------------------------------------------------------------
  JPH::Vec3 velocity = m_BodyInterface->GetLinearVelocity(bodyID);
  JPH::Vec3 moveDir = JPH::Vec3::sZero();

  if (Input::IsKeyPressed(KEY_W))
    moveDir.SetZ(-1.0f);
  if (Input::IsKeyPressed(KEY_S))
    moveDir.SetZ(1.0f);
  if (Input::IsKeyPressed(KEY_A))
    moveDir.SetX(-1.0f);
  if (Input::IsKeyPressed(KEY_D))
    moveDir.SetX(1.0f);

  if (moveDir.LengthSq() > 0.0f) {
    moveDir = moveDir.Normalized() * m_Speed;
    // Preserve vertical velocity (gravity)
    moveDir.SetY(velocity.GetY());

    // Directly setting velocity for responsive movement (Basic Character
    // Controller style) Ideally we use specialized CharacterVirtual from Jolt,
    // but explicit velocity works for simple boxes. Activate essential for
    // sleeping bodies
    m_BodyInterface->SetLinearVelocity(bodyID, moveDir);
    m_BodyInterface->ActivateBody(bodyID);
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

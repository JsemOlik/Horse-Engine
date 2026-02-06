#include <Jolt/Jolt.h>

#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Physics/PhysicsComponents.h"
#include "HorseEngine/Physics/PhysicsSystem.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <iostream>
#include <thread>

// Jolt Callback Trace
static void TraceImpl(const char *inFMT, ...) {
  // Format the message
  va_list list;
  va_start(list, inFMT);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), inFMT, list);
  va_end(list);

  // Print to the TTY
  HORSE_LOG_CORE_INFO("[Jolt] {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
// Jolt Callback Assert
static bool AssertFailedImpl(const char *inExpression, const char *inMessage,
                             const char *inFile, uint32_t inLine) {
  HORSE_LOG_CORE_ERROR("[Jolt] Assert failed: {} ({}) in {}:{}", inExpression,
                       (inMessage ? inMessage : ""), inFile, inLine);
  return true; // Breakpoint
};
#endif

namespace Horse {

// Layer constants
namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS = 2;
}; // namespace BroadPhaseLayers

// Class definitions for Jolt Interfaces
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
  virtual uint32_t GetNumBroadPhaseLayers() const override {
    return BroadPhaseLayers::NUM_LAYERS;
  }

  virtual JPH::BroadPhaseLayer
  GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
    JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    if (inLayer == Layers::NON_MOVING)
      return BroadPhaseLayers::NON_MOVING;
    return BroadPhaseLayers::MOVING;
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char *
  GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
    switch ((JPH::BroadPhaseLayer::Type)inLayer) {
    case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
      return "NON_MOVING";
    case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
      return "MOVING";
    default:
      JPH_ASSERT(false);
      return "INVALID";
    }
  }
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl
    : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
                             JPH::BroadPhaseLayer inLayer2) const override {
    switch (inLayer1) {
    case Layers::NON_MOVING:
      return inLayer2 == BroadPhaseLayers::MOVING;
    case Layers::MOVING:
      return true; // MOVING collides with everything
    default:
      JPH_ASSERT(false);
      return false;
    }
  }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
  virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
                             JPH::ObjectLayer inObject2) const override {
    switch (inObject1) {
    case Layers::NON_MOVING:
      return inObject2 ==
             Layers::MOVING; // Non-moving only collides with moving
    case Layers::MOVING:
      return true; // Moving collides with everything
    default:
      JPH_ASSERT(false);
      return false;
    }
  }
};

class MyBodyActivationListener : public JPH::BodyActivationListener {
public:
  virtual void OnBodyActivated(const JPH::BodyID &inBodyID,
                               uint64_t inBodyUserData) override {
    // Optional: Log or trigger events
  }
  virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID,
                                 uint64_t inBodyUserData) override {
    // Optional
  }
};

// PhysicsSystem Implementation

PhysicsSystem::PhysicsSystem() {}

PhysicsSystem::~PhysicsSystem() { Shutdown(); }

void PhysicsSystem::Initialize() {
  // Register allocation hook
  JPH::RegisterDefaultAllocator();

  // Setup Trace and Assert
  JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
  JPH::AssertFailed = AssertFailedImpl;
#endif

  // Create factory
  JPH::Factory::sInstance = new JPH::Factory();

  // Register standard types
  JPH::RegisterTypes();

  // Allocators
  m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10MB
  m_JobSystem = new JPH::JobSystemThreadPool(
      JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
      std::thread::hardware_concurrency() - 1);

  // Interfaces
  m_BPLayerInterface = new BPLayerInterfaceImpl();
  m_ObjectVsBPLayerFilter = new ObjectVsBroadPhaseLayerFilterImpl();
  m_ObjectLayerPairFilter = new ObjectLayerPairFilterImpl();

  // Physics System
  m_JoltSystem = new JPH::PhysicsSystem();
  m_JoltSystem->Init(
      1024, 0, 1024, 1024, *(BPLayerInterfaceImpl *)m_BPLayerInterface,
      *(ObjectVsBroadPhaseLayerFilterImpl *)m_ObjectVsBPLayerFilter,
      *(ObjectLayerPairFilterImpl *)m_ObjectLayerPairFilter);

  HORSE_LOG_CORE_INFO("Jolt Physics Initialized");
}

void PhysicsSystem::Shutdown() {
  if (m_JoltSystem) {
    delete m_JoltSystem;
    m_JoltSystem = nullptr;
  }

  if (m_BPLayerInterface) {
    delete (BPLayerInterfaceImpl *)m_BPLayerInterface;
    m_BPLayerInterface = nullptr;
  }
  if (m_ObjectVsBPLayerFilter) {
    delete (ObjectVsBroadPhaseLayerFilterImpl *)m_ObjectVsBPLayerFilter;
    m_ObjectVsBPLayerFilter = nullptr;
  }
  if (m_ObjectLayerPairFilter) {
    delete (ObjectLayerPairFilterImpl *)m_ObjectLayerPairFilter;
    m_ObjectLayerPairFilter = nullptr;
  }

  if (m_JobSystem) {
    delete m_JobSystem;
    m_JobSystem = nullptr;
  }
  if (m_TempAllocator) {
    delete m_TempAllocator;
    m_TempAllocator = nullptr;
  }

  // Cleanup factory
  if (JPH::Factory::sInstance) {
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
  }
}

JPH::BodyInterface *PhysicsSystem::GetBodyInterface() const {
  return m_JoltSystem ? &m_JoltSystem->GetBodyInterface() : nullptr;
}

PhysicsSystem::RayCastResult PhysicsSystem::RayCast(const glm::vec3 &start,
                                                    const glm::vec3 &end) {
  return RayCast(start, end, nullptr);
}

PhysicsSystem::RayCastResult
PhysicsSystem::RayCast(const glm::vec3 &start, const glm::vec3 &end,
                       void *ignoreBodyRuntimePtr) {
  if (!m_JoltSystem)
    return {};

  JPH::RVec3 from(start.x, start.y, start.z);
  JPH::Vec3 dir(end.x - start.x, end.y - start.y, end.z - start.z);
  JPH::RRayCast ray(from, dir);

  JPH::RayCastSettings settings;
  JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;

  // Body Filter to ignore self
  class IgnoreBodyFilter : public JPH::BodyFilter {
  public:
    IgnoreBodyFilter(const JPH::BodyID &ignoreID) : m_IgnoreID(ignoreID) {}
    bool ShouldCollide(const JPH::BodyID &inBodyID) const override {
      return inBodyID != m_IgnoreID;
    }

  private:
    JPH::BodyID m_IgnoreID;
  };

  JPH::BodyID ignoreID;
  if (ignoreBodyRuntimePtr) {
    ignoreID = static_cast<JPH::Body *>(ignoreBodyRuntimePtr)->GetID();
  }

  IgnoreBodyFilter bodyFilter(ignoreID);

  m_JoltSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, {},
                                              bodyFilter);

  RayCastResult result;
  if (collector.HadHit()) {
    result.Hit = true;
    result.Fraction = collector.mHit.mFraction;

    JPH::RVec3 hitPos = ray.GetPointOnRay(collector.mHit.mFraction);
    result.Position = {hitPos.GetX(), hitPos.GetY(), hitPos.GetZ()};

    // Get normal
    auto &bodyLockInterface = m_JoltSystem->GetBodyLockInterface();
    JPH::BodyLockRead lock(bodyLockInterface, collector.mHit.mBodyID);
    if (lock.Succeeded()) {
      JPH::Vec3 normal = lock.GetBody().GetWorldSpaceSurfaceNormal(
          collector.mHit.mSubShapeID2, hitPos);
      result.Normal = {normal.GetX(), normal.GetY(), normal.GetZ()};
    }
  }

  return result;
}

void PhysicsSystem::OnRuntimeStart(Scene *scene) {
  m_ContextScene = scene;

  auto &bodyInterface = m_JoltSystem->GetBodyInterface();
  const float deg2rad = 3.14159f / 180.0f;

  // Iterate all entities with RigidBody + Check collisions
  auto view = scene->GetRegistry().view<RigidBodyComponent>();
  for (auto e : view) {
    Entity entity = {e, scene};
    auto &rb = entity.GetComponent<RigidBodyComponent>();
    auto &transform = entity.GetComponent<TransformComponent>();

    // Determine Shape
    JPH::ShapeRefC shape = nullptr; // Ref counted

    if (entity.HasComponent<BoxColliderComponent>()) {
      auto &bc = entity.GetComponent<BoxColliderComponent>();

      // Jolt BoxShape is half-extents
      JPH::Vec3 halfExtent = {bc.Size[0] * 0.5f * transform.Scale[0],
                              bc.Size[1] * 0.5f * transform.Scale[1],
                              bc.Size[2] * 0.5f * transform.Scale[2]};

      // Abs to avoid negative extents from negative scale
      halfExtent = JPH::Vec3(abs(halfExtent.GetX()), abs(halfExtent.GetY()),
                             abs(halfExtent.GetZ()));

      // Apply Offset? BoxShapeSettings allows offset, or we offset Body
      // position. BoxShapeSettings doesn't have offset directly, usually
      // handled by CompoundShape if needed or setting the Center of Mass. For
      // simple BoxCollider, we assume Center is 0,0,0 relative to Body. If
      // Offset is needed, use RotatedTranslatedShapeDecorator or Compound. For
      // simplicity Phase 8: Ignored Offset or just added to Body Position
      // (wrong for rotation). Let's assume Offset is local translation.

      JPH::BoxShapeSettings boxSettings(halfExtent);
      auto boxResult = boxSettings.Create();
      if (boxResult.HasError()) {
        HORSE_LOG_CORE_ERROR("Failed to create BoxShape for Entity: {}",
                             boxResult.GetError().c_str());
        continue;
      }
      shape = boxResult.Get();
    }
    // TODO: Other shapes

    if (!shape) {
      HORSE_LOG_CORE_WARN(
          "Entity '{}' has a RigidBody but no valid Collider (BoxCollider, "
          "etc.). Physics body will NOT be created.",
          entity.GetComponent<TagComponent>().Name);
      continue;
    }

    // Create Body
    JPH::Vec3 pos = {transform.Position[0], transform.Position[1],
                     transform.Position[2]};
    JPH::Quat rot = JPH::Quat::sEulerAngles({transform.Rotation[0] * deg2rad,
                                             transform.Rotation[1] * deg2rad,
                                             transform.Rotation[2] * deg2rad});

    // Layer & Motion Type
    JPH::EMotionType motionType;
    JPH::ObjectLayer objectLayer;

    if (rb.Anchored) {
      motionType = JPH::EMotionType::Static;
      objectLayer = Layers::NON_MOVING;
    } else {
      motionType = JPH::EMotionType::Dynamic; // Or Kinematic
      objectLayer = Layers::MOVING;
    }

    JPH::BodyCreationSettings bodySettings(shape, pos, rot, motionType,
                                           objectLayer);
    bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::All;
    if (!rb.UseGravity && !rb.Anchored) {
      bodySettings.mGravityFactor = 0.0f;
    }
    bodySettings.mIsSensor = rb.IsSensor;
    // bodySettings.mLinearVelocity = ...

    JPH::Body *body = bodyInterface.CreateBody(bodySettings);
    if (body) {
      bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
      rb.RuntimeBody = body; // Store pointer

      // Set velocity if dynamic
      if (!rb.Anchored) {
        body->SetLinearVelocity(
            {rb.LinearVelocity[0], rb.LinearVelocity[1], rb.LinearVelocity[2]});
        body->SetAngularVelocity({rb.AngularVelocity[0], rb.AngularVelocity[1],
                                  rb.AngularVelocity[2]});
      }
    }
  }

  // Build physics
  m_JoltSystem->OptimizeBroadPhase();
}

void PhysicsSystem::OnRuntimeStop() {
  if (!m_ContextScene)
    return;

  auto &bodyInterface = m_JoltSystem->GetBodyInterface();

  // Remove all bodies
  // Using registry to find them.
  // Or remove all from Jolt directly? Jolt has no "RemoveAll".
  // We iterate components again.

  auto view = m_ContextScene->GetRegistry().view<RigidBodyComponent>();
  for (auto e : view) {
    auto &rb = view.get<RigidBodyComponent>(e);
    if (rb.RuntimeBody) {
      JPH::Body *body = (JPH::Body *)rb.RuntimeBody;
      bodyInterface.RemoveBody(body->GetID());
      bodyInterface.DestroyBody(body->GetID());
      rb.RuntimeBody = nullptr;
    }
  }

  m_ContextScene = nullptr;
}

void PhysicsSystem::Step(float dt) {
  if (!m_ContextScene)
    return;

  // Jolt Physics Step
  // cCollisionSteps = 1, cIntegrationSubSteps = 1 for now
  m_JoltSystem->Update(dt, 1, m_TempAllocator, m_JobSystem);

  // Sync back to transforms
  const float rad2deg = 180.0f / 3.14159f;
  auto &bodyInterface = m_JoltSystem->GetBodyInterface();

  auto view = m_ContextScene->GetRegistry()
                  .view<RigidBodyComponent, TransformComponent>();
  for (auto e : view) {
    auto [rb, transform] = view.get<RigidBodyComponent, TransformComponent>(e);

    if (rb.RuntimeBody && !rb.Anchored) {
      JPH::Body *body = (JPH::Body *)rb.RuntimeBody;
      if (body->IsActive()) {
        JPH::RVec3 position = body->GetPosition();
        JPH::Quat rotation = body->GetRotation();

        transform.Position = {(float)position.GetX(), (float)position.GetY(),
                              (float)position.GetZ()};

        JPH::Vec3 euler = rotation.GetEulerAngles();
        transform.Rotation = {euler.GetX() * rad2deg, euler.GetY() * rad2deg,
                              euler.GetZ() * rad2deg};

        JPH::Vec3 linVel = body->GetLinearVelocity();
        JPH::Vec3 angVel = body->GetAngularVelocity();
        rb.LinearVelocity = {linVel.GetX(), linVel.GetY(), linVel.GetZ()};
        rb.AngularVelocity = {angVel.GetX(), angVel.GetY(), angVel.GetZ()};
      }
    }
  }
}

} // namespace Horse

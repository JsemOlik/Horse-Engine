#pragma once

#include "HorseEngine/Core.h"

// Forward declarations to avoid exposing Jolt headers everywhere
namespace JPH {
class PhysicsSystem;
class TempAllocator;
class JobSystem;
class BodyInterface;
} // namespace JPH

namespace Horse {

class Scene;

class PhysicsSystem {
public:
  PhysicsSystem();
  ~PhysicsSystem();

  void Initialize();
  void Shutdown();

  // Runtime Loop
  void OnRuntimeStart(Scene *scene);
  void OnRuntimeStop();
  void Step(float dt);

  // Accessors (for internal use or advanced users)
  JPH::PhysicsSystem *GetJoltSystem() const { return m_JoltSystem; }
  JPH::BodyInterface *GetBodyInterface() const;

private:
  JPH::PhysicsSystem *m_JoltSystem = nullptr;
  JPH::TempAllocator *m_TempAllocator = nullptr;
  JPH::JobSystem *m_JobSystem = nullptr;

  // Jolt interfaces (stored as opaque pointers or simple internal classes if
  // defined in cpp) We'll define them in the cpp or use void* if we want to be
  // strict, but here we just keep members private. For simplicity with PIMPL or
  // just member pointers:
  void *m_BPLayerInterface = nullptr;
  void *m_ObjectVsBPLayerFilter = nullptr;
  void *m_ObjectLayerPairFilter = nullptr;

  Scene *m_ContextScene = nullptr;
};

} // namespace Horse

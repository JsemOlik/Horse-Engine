#include "HorseEngine/Scene/Scene.h"
#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Physics/PhysicsSystem.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/SceneSerializer.h"
#include "HorseEngine/Scene/ScriptableEntity.h"
#include "HorseEngine/Scripting/LuaScriptEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Horse {

std::shared_ptr<Scene> Scene::Copy(const std::shared_ptr<Scene> &other) {
  if (!other)
    return nullptr;

  std::string json = SceneSerializer::SerializeToJSONString(other.get());
  return SceneSerializer::DeserializeFromJSONString(json);
}

Scene::Scene(const std::string &name) : m_Name(name) {
  m_PhysicsSystem = new PhysicsSystem();
  m_PhysicsSystem->Initialize();
}

Scene::~Scene() {
  if (m_PhysicsSystem) {
    m_PhysicsSystem->Shutdown();
    delete m_PhysicsSystem;
    m_PhysicsSystem = nullptr;
  }
}

Entity Scene::CreateEntity(const std::string &name) {
  return CreateEntityWithUUID(UUID(), name);
}

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name) {
  Entity entity = {m_Registry.create(), this};

  entity.AddComponent<UUIDComponent>(uuid);
  entity.AddComponent<TagComponent>(name);
  entity.AddComponent<TransformComponent>();
  entity.AddComponent<RelationshipComponent>();

  m_EntityMap[uuid] = entity.GetHandle();

  HORSE_LOG_CORE_INFO("Created entity: {} (UUID: {})", name, uuid.ToString());

  return entity;
}

void Scene::DestroyEntity(Entity entity) {
  if (!entity)
    return;

  // Remove from parent's children list
  if (entity.HasComponent<RelationshipComponent>()) {
    RemoveParent(entity);
  }

  // Find and remove from entity map
  for (auto it = m_EntityMap.begin(); it != m_EntityMap.end(); ++it) {
    if (it->second == entity.GetHandle()) {
      m_EntityMap.erase(it);
      break;
    }
  }

  m_Registry.destroy(entity.GetHandle());
}

Entity Scene::GetEntityByUUID(UUID uuid) {
  auto it = m_EntityMap.find(uuid);
  if (it != m_EntityMap.end()) {
    return {it->second, this};
  }
  return {};
}

Entity Scene::GetEntityByName(const std::string &name) {
  auto view = m_Registry.view<TagComponent>();
  for (auto entity : view) {
    const auto &tag = view.get<TagComponent>(entity);
    if (tag.Name == name) {
      return {entity, this};
    }
  }
  return {};
}

void Scene::SetEntityParent(Entity child, Entity parent) {
  if (!child)
    return;

  // Remove from current parent first
  RemoveParent(child);

  if (!parent)
    return;

  auto &childRel = child.GetComponent<RelationshipComponent>();
  auto &parentRel = parent.GetComponent<RelationshipComponent>();

  childRel.Parent = parent.GetHandle();

  // Add to parent's children list
  if (parentRel.FirstChild == entt::null) {
    // Parent has no children, make this the first child
    parentRel.FirstChild = child.GetHandle();
  } else {
    // Find the last child and add as sibling
    Entity lastChild = {parentRel.FirstChild, this};
    while (lastChild.GetComponent<RelationshipComponent>().NextSibling !=
           entt::null) {
      lastChild = {lastChild.GetComponent<RelationshipComponent>().NextSibling,
                   this};
    }

    auto &lastChildRel = lastChild.GetComponent<RelationshipComponent>();
    lastChildRel.NextSibling = child.GetHandle();
    childRel.PrevSibling = lastChild.GetHandle();
  }
}

void Scene::RemoveParent(Entity child) {
  if (!child || !child.HasComponent<RelationshipComponent>())
    return;

  auto &childRel = child.GetComponent<RelationshipComponent>();

  if (childRel.Parent == entt::null)
    return;

  Entity parent = {childRel.Parent, this};
  auto &parentRel = parent.GetComponent<RelationshipComponent>();

  // Update parent's first child if this is the first child
  if (parentRel.FirstChild == child.GetHandle()) {
    parentRel.FirstChild = childRel.NextSibling;
  }

  // Update siblings
  if (childRel.PrevSibling != entt::null) {
    Entity prevSibling = {childRel.PrevSibling, this};
    prevSibling.GetComponent<RelationshipComponent>().NextSibling =
        childRel.NextSibling;
  }

  if (childRel.NextSibling != entt::null) {
    Entity nextSibling = {childRel.NextSibling, this};
    nextSibling.GetComponent<RelationshipComponent>().PrevSibling =
        childRel.PrevSibling;
  }

  // Clear child's relationship
  childRel.Parent = entt::null;
  childRel.PrevSibling = entt::null;
  childRel.NextSibling = entt::null;
}

Entity Scene::GetParent(Entity entity) {
  if (!entity || !entity.HasComponent<RelationshipComponent>())
    return {};

  auto &rel = entity.GetComponent<RelationshipComponent>();
  if (rel.Parent == entt::null)
    return {};

  return {rel.Parent, this};
}

void Scene::OnRuntimeStart() {
  if (m_PhysicsSystem)
    m_PhysicsSystem->OnRuntimeStart(this);

  // Lock cursor
  Input::SetCursorMode(CursorMode::Locked);

  m_State = SceneState::Loading;
  m_LoadingStage = LoadingStage::Assets;
  TriggerAssetLoads();
}

void Scene::TriggerAssetLoads() {
  m_LoadingQueue.clear();

  auto view = m_Registry.view<MeshRendererComponent>();
  for (auto entity : view) {
    auto &mesh = view.get<MeshRendererComponent>(entity);
    if (!mesh.MeshGUID.empty()) {
      m_LoadingQueue.push_back(mesh.MeshGUID);
    }
  }

  HORSE_LOG_CORE_INFO("Triggered asset loading for {} assets.",
                      m_LoadingQueue.size());
}

void Scene::UpdateStagedLoad() {
  switch (m_LoadingStage) {
  case LoadingStage::Assets:
    // Check if all queued assets are ready (simulated for now)
    if (m_LoadingQueue.empty()) {
      m_LoadingStage = LoadingStage::Components;
      HORSE_LOG_CORE_INFO(
          "Asset loading complete. Transitioning to Components stage.");
    } else {
      // Simulate asynchronous loading by popping one asset per update
      m_LoadingQueue.pop_back();
    }
    break;

  case LoadingStage::Components:
    // Initialize components if needed
    m_LoadingStage = LoadingStage::Scripts;
    HORSE_LOG_CORE_INFO(
        "Component initialization complete. Transitioning to Scripts stage.");
    break;

  case LoadingStage::Scripts:
    // 1. Awake
    {
      auto view = m_Registry.view<ScriptComponent>();
      for (auto entity : view) {
        auto &script = view.get<ScriptComponent>(entity);
        if (!script.AwakeCalled) {
          HORSE_LOG_CORE_INFO("Awaking entity {}...",
                              m_Registry.get<TagComponent>(entity).Name);
          script.AwakeCalled = true;
          LuaScriptEngine::OnCreateEntity({entity, this});
        }
      }
    }

    // 2. Start
    {
      auto view = m_Registry.view<ScriptComponent>();
      for (auto entity : view) {
        auto &script = view.get<ScriptComponent>(entity);
        if (!script.StartCalled) {
          HORSE_LOG_CORE_INFO("Starting entity {}...",
                              m_Registry.get<TagComponent>(entity).Name);
          script.StartCalled = true;
          // TODO: ScriptEngine::OnStart(entity)
        }
      }
    }

    {
      auto view = m_Registry.view<NativeScriptComponent>();
      for (auto entity : view) {
        auto &script = view.get<NativeScriptComponent>(entity);
        if (!script.Instance && script.InstantiateScript) {
          script.Instance = script.InstantiateScript();
          script.Instance->m_Entity = Entity{entity, this};
          script.Instance->OnCreate();
        }
      }
    }

    m_LoadingStage = LoadingStage::Ready;
    m_State = SceneState::Play;
    HORSE_LOG_CORE_INFO("Scene stage Ready. State transitioned to Play.");
    break;
  }
}

void Scene::OnRuntimeStop() {
  if (m_PhysicsSystem)
    m_PhysicsSystem->OnRuntimeStop();

  Input::SetCursorMode(CursorMode::Normal);

  m_State = SceneState::Edit;

  // Reset script states
  {
    auto view = m_Registry.view<ScriptComponent>();
    for (auto entity : view) {
      auto &script = view.get<ScriptComponent>(entity);
      script.AwakeCalled = false;
      script.StartCalled = false;
      LuaScriptEngine::OnDestroyEntity({entity, this});
    }
  }

  // Native Scripts
  {
    auto view = m_Registry.view<NativeScriptComponent>();
    for (auto entity : view) {
      auto &nsc = view.get<NativeScriptComponent>(entity);
      if (nsc.Instance) {
        nsc.Instance->OnDestroy();
        nsc.DestroyScript(&nsc);
      }
    }
  }
}

void Scene::OnRuntimeUpdate(float deltaTime) {
  if (m_State == SceneState::Play) {
    // Input Handling for Cursor Mode
    if (Input::IsKeyPressed(KEY_ESCAPE)) {
      Input::SetCursorMode(CursorMode::Normal);
    }
    if (Input::IsMouseButtonPressed(KEY_LBUTTON) &&
        Input::GetCursorMode() == CursorMode::Normal) {
      Input::SetCursorMode(CursorMode::Locked);
    }

    // 3. Update scripts
    {
      auto view = m_Registry.view<ScriptComponent>();
      for (auto entity : view) {
        LuaScriptEngine::OnUpdateEntity({entity, this}, deltaTime);
      }
    }

    {
      auto view = m_Registry.view<NativeScriptComponent>();
      for (auto entity : view) {
        auto &nsc = view.get<NativeScriptComponent>(entity);
        if (nsc.Instance) {
          nsc.Instance->OnUpdate(deltaTime);
        }
      }
    }

    // 4. Physics Step
    if (m_PhysicsSystem) {
      m_PhysicsSystem->Step(deltaTime);
    }
  }

  UpdateTransformHierarchy();
}

void Scene::OnUpdate(float deltaTime) {
  if (m_State == SceneState::Edit) {
    UpdateTransformHierarchy();
  } else if (m_State == SceneState::Loading) {
    UpdateStagedLoad();
  } else if (m_State == SceneState::Play) {
    OnRuntimeUpdate(deltaTime);
  }
}

void Scene::UpdateTransformHierarchy() {
  auto view = m_Registry.view<TransformComponent, RelationshipComponent>();
  for (auto entity : view) {
    auto &relationship = view.get<RelationshipComponent>(entity);
    // Find roots (entities with no parent)
    if (relationship.Parent == entt::null) {
      UpdateEntityTransform({entity, this}, glm::mat4(1.0f));
    }
  }
}

void Scene::UpdateEntityTransform(Entity entity,
                                  const glm::mat4 &parentTransform) {
  if (!entity.HasComponent<TransformComponent>())
    return;

  auto &transform = entity.GetComponent<TransformComponent>();

  // Calculate Local Matrix
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::make_vec3(transform.Position.data()));
  model = glm::rotate(model, glm::radians(transform.Rotation[0]),
                      glm::vec3(1, 0, 0));
  model = glm::rotate(model, glm::radians(transform.Rotation[1]),
                      glm::vec3(0, 1, 0));
  model = glm::rotate(model, glm::radians(transform.Rotation[2]),
                      glm::vec3(0, 0, 1));
  model = glm::scale(model, glm::make_vec3(transform.Scale.data()));

  // Calculate World Matrix
  transform.WorldTransform = parentTransform * model;

  // Propagate to children
  if (entity.HasComponent<RelationshipComponent>()) {
    auto &rel = entity.GetComponent<RelationshipComponent>();
    entt::entity childHandle = rel.FirstChild;
    while (childHandle != entt::null) {
      Entity child(childHandle, this);
      UpdateEntityTransform(child, transform.WorldTransform);
      childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
    }
  }
}

} // namespace Horse

#include "HorseEngine/Scene/Scene.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Scene/Components.h"

namespace Horse {

Scene::Scene(const std::string &name) : m_Name(name) {}

Scene::~Scene() {}

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
  if (!child || !parent)
    return;

  // Remove from current parent first
  RemoveParent(child);

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

void Scene::OnUpdate(float deltaTime) {
  // Update transform hierarchy
  UpdateTransformHierarchy();
}

void Scene::UpdateTransformHierarchy() {
  // TODO: Implement transform propagation from parent to children
  // This will be needed when we add actual rendering
}

} // namespace Horse

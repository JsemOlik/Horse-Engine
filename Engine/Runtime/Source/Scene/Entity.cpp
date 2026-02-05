#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"

namespace Horse {

Entity::Entity(entt::entity handle, Scene *scene)
    : m_EntityHandle(handle), m_Scene(scene) {}

UUID Entity::GetUUID() { return GetComponent<UUIDComponent>().ID; }

} // namespace Horse

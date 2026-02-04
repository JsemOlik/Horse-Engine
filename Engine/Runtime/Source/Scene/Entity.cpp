#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"

namespace Horse {

Entity::Entity(entt::entity handle, Scene *scene)
    : m_EntityHandle(handle), m_Scene(scene) {}

} // namespace Horse

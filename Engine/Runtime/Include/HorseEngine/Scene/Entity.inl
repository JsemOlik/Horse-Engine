#pragma once

namespace Horse {

template <typename T, typename... Args>
T &Entity::AddComponent(Args &&...args) {
  return m_Scene->GetRegistry().emplace<T>(m_EntityHandle,
                                           std::forward<Args>(args)...);
}

template <typename T> T &Entity::GetComponent() {
  return m_Scene->GetRegistry().get<T>(m_EntityHandle);
}

template <typename T> bool Entity::HasComponent() {
  return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
}

template <typename T> void Entity::RemoveComponent() {
  m_Scene->GetRegistry().remove<T>(m_EntityHandle);
}

} // namespace Horse

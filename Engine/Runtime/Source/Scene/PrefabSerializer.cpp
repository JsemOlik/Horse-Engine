#include "HorseEngine/Scene/PrefabSerializer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Horse {

// These are duplicated from SceneSerializer for now.
// Ideally these would be in a shared ComponentSerializer utility.
static json SerializeEntityTemplate(Entity entity) {
  json entityJson;

  // Prefabs use relative hierarchy, so we don't necessarily need the original
  // UUIDs but keeping them for consistency and local children mapping is fine.
  if (entity.HasComponent<UUIDComponent>()) {
    entityJson["uuid"] =
        std::to_string(entity.GetComponent<UUIDComponent>().ID);
  }

  json componentsJson;

  if (entity.HasComponent<TagComponent>()) {
    auto &c = entity.GetComponent<TagComponent>();
    componentsJson["TagComponent"] = {{"name", c.Name}, {"tag", c.Tag}};
  }

  if (entity.HasComponent<TransformComponent>()) {
    auto &c = entity.GetComponent<TransformComponent>();
    componentsJson["TransformComponent"] = {
        {"position", c.Position}, {"rotation", c.Rotation}, {"scale", c.Scale}};
  }

  if (entity.HasComponent<CameraComponent>()) {
    auto &c = entity.GetComponent<CameraComponent>();
    componentsJson["CameraComponent"] = {
        {"type", c.Type == CameraComponent::ProjectionType::Perspective
                     ? "Perspective"
                     : "Orthographic"},
        {"fov", c.FOV},
        {"nearClip", c.NearClip},
        {"farClip", c.FarClip},
        {"primary", c.Primary}};
  }

  if (entity.HasComponent<MeshRendererComponent>()) {
    auto &c = entity.GetComponent<MeshRendererComponent>();
    componentsJson["MeshRendererComponent"] = {
        {"meshGuid", c.MeshGUID}, {"materialGuid", c.MaterialGUID}};
  }

  if (entity.HasComponent<ScriptComponent>()) {
    auto &c = entity.GetComponent<ScriptComponent>();
    componentsJson["ScriptComponent"] = {{"scriptGuid", c.ScriptGUID}};
  }

  // Note: We don't serialize RelationshipComponent directly in the template
  // Instead we serialize children recursively.
  entityJson["components"] = componentsJson;

  // Serialize children
  json childrenJson = json::array();
  if (entity.HasComponent<RelationshipComponent>()) {
    auto &rel = entity.GetComponent<RelationshipComponent>();
    entt::entity childHandle = rel.FirstChild;
    while (childHandle != entt::null) {
      Entity child(childHandle, entity.GetScene());
      childrenJson.push_back(SerializeEntityTemplate(child));

      if (child.HasComponent<RelationshipComponent>()) {
        childHandle = child.GetComponent<RelationshipComponent>().NextSibling;
      } else {
        break;
      }
    }
  }
  entityJson["children"] = childrenJson;

  return entityJson;
}

bool PrefabSerializer::SerializeToJSON(const Prefab *prefab,
                                       const std::string &filepath) {
  if (!prefab)
    return false;

  try {
    json prefabJson;
    prefabJson["name"] = prefab->GetName();
    prefabJson["template"] = prefab->GetTemplateData();

    std::ofstream file(filepath);
    if (!file.is_open())
      return false;

    file << prefabJson.dump(2);
    file.close();
    return true;
  } catch (...) {
    return false;
  }
}

std::shared_ptr<Prefab>
PrefabSerializer::DeserializeFromJSON(const std::string &filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open())
      return nullptr;

    json prefabJson;
    file >> prefabJson;
    file.close();

    auto prefab = std::make_shared<Prefab>();
    prefab->SetName(prefabJson.value("name", "Untitled Prefab"));
    prefab->SetTemplateData(prefabJson["template"]);
    return prefab;
  } catch (...) {
    return nullptr;
  }
}

std::shared_ptr<Prefab>
PrefabSerializer::CreateFromEntity(Entity entity, const std::string &name) {
  if (!entity)
    return nullptr;

  auto prefab = std::make_shared<Prefab>();
  prefab->SetName(name);
  prefab->SetTemplateData(SerializeEntityTemplate(entity));
  return prefab;
}

} // namespace Horse

#include "HorseEngine/Scene/SceneSerializer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"
#include "HorseEngine/Scene/UUID.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Horse {

// Helper functions for component serialization
static json SerializeTagComponent(const TagComponent &comp) {
  return {{"name", comp.Name}, {"tag", comp.Tag}};
}

static void DeserializeTagComponent(const json &j, TagComponent &comp) {
  comp.Name = j.value("name", "Entity");
  comp.Tag = j.value("tag", "Default");
}

static json SerializeTransformComponent(const TransformComponent &comp) {
  return {{"position", comp.Position},
          {"rotation", comp.Rotation},
          {"scale", comp.Scale}};
}

static void DeserializeTransformComponent(const json &j,
                                          TransformComponent &comp) {
  if (j.contains("position"))
    comp.Position = j["position"].get<std::array<float, 3>>();
  if (j.contains("rotation"))
    comp.Rotation = j["rotation"].get<std::array<float, 3>>();
  if (j.contains("scale"))
    comp.Scale = j["scale"].get<std::array<float, 3>>();
}

static json SerializeRelationshipComponent(const RelationshipComponent &comp,
                                           Scene *scene) {
  json j;

  if (comp.Parent != entt::null) {
    Entity parent(comp.Parent, scene);
    if (parent.HasComponent<UUIDComponent>()) {
      j["parent"] = std::to_string(parent.GetComponent<UUIDComponent>().ID);
    }
  } else {
    j["parent"] = nullptr;
  }

  if (comp.FirstChild != entt::null) {
    Entity child(comp.FirstChild, scene);
    if (child.HasComponent<UUIDComponent>()) {
      j["firstChild"] = std::to_string(child.GetComponent<UUIDComponent>().ID);
    }
  } else {
    j["firstChild"] = nullptr;
  }

  if (comp.NextSibling != entt::null) {
    Entity sibling(comp.NextSibling, scene);
    if (sibling.HasComponent<UUIDComponent>()) {
      j["nextSibling"] =
          std::to_string(sibling.GetComponent<UUIDComponent>().ID);
    }
  } else {
    j["nextSibling"] = nullptr;
  }

  return j;
}

static json SerializeCameraComponent(const CameraComponent &comp) {
  return {{"type", comp.Type == CameraComponent::ProjectionType::Perspective
                       ? "Perspective"
                       : "Orthographic"},
          {"fov", comp.FOV},
          {"nearClip", comp.NearClip},
          {"farClip", comp.FarClip},
          {"primary", comp.Primary}};
}

static void DeserializeCameraComponent(const json &j, CameraComponent &comp) {
  std::string type = j.value("type", "Perspective");
  comp.Type = (type == "Perspective")
                  ? CameraComponent::ProjectionType::Perspective
                  : CameraComponent::ProjectionType::Orthographic;
  comp.FOV = j.value("fov", 45.0f);
  comp.NearClip = j.value("nearClip", 0.1f);
  comp.FarClip = j.value("farClip", 1000.0f);
  comp.Primary = j.value("primary", false);
}

static json SerializeLightComponent(const LightComponent &comp) {
  std::string typeStr;
  switch (comp.Type) {
  case LightComponent::LightType::Directional:
    typeStr = "Directional";
    break;
  case LightComponent::LightType::Point:
    typeStr = "Point";
    break;
  case LightComponent::LightType::Spot:
    typeStr = "Spot";
    break;
  }

  return {
      {"type", typeStr}, {"color", comp.Color}, {"intensity", comp.Intensity}};
}

static void DeserializeLightComponent(const json &j, LightComponent &comp) {
  std::string type = j.value("type", "Directional");
  if (type == "Directional")
    comp.Type = LightComponent::LightType::Directional;
  else if (type == "Point")
    comp.Type = LightComponent::LightType::Point;
  else if (type == "Spot")
    comp.Type = LightComponent::LightType::Spot;

  if (j.contains("color"))
    comp.Color = j["color"].get<std::array<float, 3>>();
  comp.Intensity = j.value("intensity", 1.0f);
}

static json SerializeMeshRendererComponent(const MeshRendererComponent &comp) {
  return {{"meshGuid", comp.MeshGUID}, {"materialGuid", comp.MaterialGUID}};
}

static void DeserializeMeshRendererComponent(const json &j,
                                             MeshRendererComponent &comp) {
  comp.MeshGUID = j.value("meshGuid", "");
  comp.MaterialGUID = j.value("materialGuid", "");
}

static json SerializeScriptComponent(const ScriptComponent &comp) {
  return {{"scriptGuid", comp.ScriptGUID}};
}

static void DeserializeScriptComponent(const json &j, ScriptComponent &comp) {
  comp.ScriptGUID = j.value("scriptGuid", "");
}

static json SerializePrefabComponent(const PrefabComponent &comp) {
  return {{"prefabGuid", comp.PrefabGUID}, {"overrides", comp.Overrides}};
}

static void DeserializePrefabComponent(const json &j, PrefabComponent &comp) {
  comp.PrefabGUID = j.value("prefabGuid", "");
  comp.Overrides = j.value("overrides", "");
}

// Helper functions for full scene serialization
static json SerializeSceneToJson(const Scene *scene) {
  json sceneJson;
  sceneJson["name"] = scene->GetName();
  sceneJson["version"] = "1.0.0";
  sceneJson["entities"] = json::array();

  auto &registry = scene->GetRegistry();
  auto view = registry.view<UUIDComponent>();

  for (auto entityHandle : view) {
    Entity entity(entityHandle, const_cast<Scene *>(scene));
    json entityJson;

    // Serialize UUID
    auto &uuid = entity.GetComponent<UUIDComponent>();
    entityJson["uuid"] = std::to_string(uuid.ID);

    // Serialize components
    json componentsJson;

    if (entity.HasComponent<TagComponent>()) {
      componentsJson["TagComponent"] =
          SerializeTagComponent(entity.GetComponent<TagComponent>());
    }

    if (entity.HasComponent<TransformComponent>()) {
      componentsJson["TransformComponent"] = SerializeTransformComponent(
          entity.GetComponent<TransformComponent>());
    }

    if (entity.HasComponent<RelationshipComponent>()) {
      componentsJson["RelationshipComponent"] = SerializeRelationshipComponent(
          entity.GetComponent<RelationshipComponent>(),
          const_cast<Scene *>(scene));
    }

    if (entity.HasComponent<CameraComponent>()) {
      componentsJson["CameraComponent"] =
          SerializeCameraComponent(entity.GetComponent<CameraComponent>());
    }

    if (entity.HasComponent<LightComponent>()) {
      componentsJson["LightComponent"] =
          SerializeLightComponent(entity.GetComponent<LightComponent>());
    }

    if (entity.HasComponent<MeshRendererComponent>()) {
      componentsJson["MeshRendererComponent"] = SerializeMeshRendererComponent(
          entity.GetComponent<MeshRendererComponent>());
    }

    if (entity.HasComponent<ScriptComponent>()) {
      componentsJson["ScriptComponent"] =
          SerializeScriptComponent(entity.GetComponent<ScriptComponent>());
    }

    if (entity.HasComponent<PrefabComponent>()) {
      componentsJson["PrefabComponent"] =
          SerializePrefabComponent(entity.GetComponent<PrefabComponent>());
    }

    entityJson["components"] = componentsJson;
    sceneJson["entities"].push_back(entityJson);
  }
  return sceneJson;
}

static std::shared_ptr<Scene> DeserializeSceneFromJson(const json &sceneJson) {
  // Create scene
  std::string sceneName = sceneJson.value("name", "Untitled Scene");
  auto scene = std::make_shared<Scene>(sceneName);

  // First pass: Create all entities with UUIDs
  std::unordered_map<UUID, entt::entity> uuidToEntity;

  for (const auto &entityJson : sceneJson["entities"]) {
    UUID uuid(std::stoull(entityJson["uuid"].get<std::string>()));

    // Create entity with specific UUID
    auto entity = scene->CreateEntityWithUUID(uuid, "Temp");
    uuidToEntity[uuid] = entity.GetHandle();
  }

  // Second pass: Deserialize components
  for (const auto &entityJson : sceneJson["entities"]) {
    UUID uuid(std::stoull(entityJson["uuid"].get<std::string>()));
    Entity entity(uuidToEntity[uuid], scene.get());

    const auto &componentsJson = entityJson["components"];

    // Tag Component (always present)
    if (componentsJson.contains("TagComponent")) {
      auto &tag = entity.GetComponent<TagComponent>();
      DeserializeTagComponent(componentsJson["TagComponent"], tag);
    }

    // Transform Component
    if (componentsJson.contains("TransformComponent")) {
      if (!entity.HasComponent<TransformComponent>()) {
        entity.AddComponent<TransformComponent>();
      }
      auto &transform = entity.GetComponent<TransformComponent>();
      DeserializeTransformComponent(componentsJson["TransformComponent"],
                                    transform);
    }

    // Camera Component
    if (componentsJson.contains("CameraComponent")) {
      auto &camera = entity.AddComponent<CameraComponent>();
      DeserializeCameraComponent(componentsJson["CameraComponent"], camera);
    }

    // Light Component
    if (componentsJson.contains("LightComponent")) {
      auto &light = entity.AddComponent<LightComponent>();
      DeserializeLightComponent(componentsJson["LightComponent"], light);
    }

    // MeshRenderer Component
    if (componentsJson.contains("MeshRendererComponent")) {
      auto &meshRenderer = entity.AddComponent<MeshRendererComponent>();
      DeserializeMeshRendererComponent(componentsJson["MeshRendererComponent"],
                                       meshRenderer);
    }

    // Script Component
    if (componentsJson.contains("ScriptComponent")) {
      auto &script = entity.AddComponent<ScriptComponent>();
      DeserializeScriptComponent(componentsJson["ScriptComponent"], script);
    }

    // Prefab Component
    if (componentsJson.contains("PrefabComponent")) {
      auto &prefab = entity.AddComponent<PrefabComponent>();
      DeserializePrefabComponent(componentsJson["PrefabComponent"], prefab);
    }
  }

  // Third pass: Restore relationships
  for (const auto &entityJson : sceneJson["entities"]) {
    if (!entityJson["components"].contains("RelationshipComponent"))
      continue;

    UUID uuid(std::stoull(entityJson["uuid"].get<std::string>()));
    Entity entity(uuidToEntity[uuid], scene.get());

    const auto &relJson = entityJson["components"]["RelationshipComponent"];

    // Restore parent relationship
    if (!relJson["parent"].is_null()) {
      UUID parentUUID(std::stoull(relJson["parent"].get<std::string>()));
      if (uuidToEntity.find(parentUUID) != uuidToEntity.end()) {
        Entity parent(uuidToEntity[parentUUID], scene.get());
        scene->SetEntityParent(entity, parent);
      }
    }
  }
  return scene;
}

// Main serialization functions
bool SceneSerializer::SerializeToJSON(const Scene *scene,
                                      const std::string &filepath) {
  if (!scene) {
    HORSE_LOG_CORE_ERROR("Cannot serialize null scene");
    return false;
  }

  try {
    json sceneJson = SerializeSceneToJson(scene);

    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open()) {
      HORSE_LOG_CORE_ERROR("Failed to open file for writing: {}", filepath);
      return false;
    }

    file << sceneJson.dump(2); // Pretty print with 2-space indent
    file.close();

    HORSE_LOG_CORE_INFO("Scene serialized successfully to: {}", filepath);
    return true;

  } catch (const std::exception &e) {
    HORSE_LOG_CORE_ERROR("Failed to serialize scene: {}", e.what());
    return false;
  }
}

std::string SceneSerializer::SerializeToJSONString(const Scene *scene) {
  if (!scene)
    return "";
  try {
    return SerializeSceneToJson(scene).dump();
  } catch (...) {
    return "";
  }
}

std::shared_ptr<Scene>
SceneSerializer::DeserializeFromJSON(const std::string &filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      HORSE_LOG_CORE_ERROR("Failed to open file for reading: {}", filepath);
      return nullptr;
    }

    json sceneJson;
    file >> sceneJson;
    file.close();

    auto scene = DeserializeSceneFromJson(sceneJson);
    HORSE_LOG_CORE_INFO("Scene deserialized successfully from: {}", filepath);
    return scene;

  } catch (const std::exception &e) {
    HORSE_LOG_CORE_ERROR("Failed to deserialize scene: {}", e.what());
    return nullptr;
  }
}

std::shared_ptr<Scene>
SceneSerializer::DeserializeFromJSONString(const std::string &jsonString) {
  try {
    json sceneJson = json::parse(jsonString);
    return DeserializeSceneFromJson(sceneJson);
  } catch (...) {
    return nullptr;
  }
}

} // namespace Horse

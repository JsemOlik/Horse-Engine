#pragma once

#include <memory>
#include <string>

namespace Horse {
class Scene;

class SceneSerializer {
public:
  SceneSerializer() = delete;

  // JSON Serialization
  static bool SerializeToJSON(const Scene *scene, const std::string &filepath);
  static std::shared_ptr<Scene>
  DeserializeFromJSON(const std::string &filepath);

  // In-memory JSON Serialization (for cloning)
  static std::string SerializeToJSONString(const Scene *scene);
  static std::shared_ptr<Scene>
  DeserializeFromJSONString(const std::string &jsonString);

  // Binary Serialization (future)
  // static bool SerializeToBinary(const Scene* scene, const std::string&
  // filepath); static std::shared_ptr<Scene> DeserializeFromBinary(const
  // std::string& filepath);
};
} // namespace Horse

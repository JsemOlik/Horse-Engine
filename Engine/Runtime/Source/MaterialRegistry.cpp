#include "HorseEngine/Render/MaterialRegistry.h"
#include "HorseEngine/Core/Logging.h"

namespace Horse {

MaterialRegistry &MaterialRegistry::Get() {
  static MaterialRegistry instance;
  return instance;
}

MaterialRegistry::MaterialRegistry() {
  // Create Default Material
  auto defaultMat = std::make_shared<Material>("Default");
  defaultMat->SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
  defaultMat->SetFloat("Roughness", 0.5f);
  defaultMat->SetFloat("Metalness", 0.0f);
  m_Materials["Default"] = defaultMat;
}

std::shared_ptr<Material>
MaterialRegistry::GetMaterial(const std::string &name) {
  auto it = m_Materials.find(name);
  if (it != m_Materials.end()) {
    return it->second;
  }
  return m_Materials["Default"];
}

std::shared_ptr<Material>
MaterialRegistry::CreateMaterial(const std::string &name) {
  if (m_Materials.find(name) != m_Materials.end()) {
    HORSE_LOG_CORE_WARN(
        "Material '{}' already exists, returning existing instance", name);
    return m_Materials[name];
  }

  auto newMat = std::make_shared<Material>(name);
  // Default values
  newMat->SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
  newMat->SetFloat("Roughness", 0.5f);
  newMat->SetFloat("Metalness", 0.0f);

  m_Materials[name] = newMat;
  return newMat;
}

} // namespace Horse

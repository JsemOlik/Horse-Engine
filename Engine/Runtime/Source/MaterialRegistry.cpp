#include "HorseEngine/Render/MaterialRegistry.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Render/MaterialSerializer.h"
#include <filesystem>

namespace Horse {

MaterialRegistry &MaterialRegistry::Get() {
  static MaterialRegistry instance;
  return instance;
}

MaterialRegistry::MaterialRegistry() {
  // Create Default Material
  auto defaultMat = std::make_shared<MaterialInstance>("Default");
  defaultMat->SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
  defaultMat->SetFloat("Roughness", 0.5f);
  defaultMat->SetFloat("Metalness", 0.0f);
  m_Materials["Default"] = defaultMat;
}

std::shared_ptr<MaterialInstance>
MaterialRegistry::GetMaterial(const std::string &nameOrGuid) {
  auto it = m_Materials.find(nameOrGuid);
  if (it != m_Materials.end()) {
    return it->second;
  }

  // If not found by name, try to treat as GUID and load via AssetManager
  try {
    UUID guid(0);
    try {
      guid = UUID(std::stoull(nameOrGuid));
    } catch (...) {
      // Not a numeric GUID, try friendly name via AssetManager
      auto &am = AssetManager::Get();
      guid = am.GetHandleByFriendlyName(nameOrGuid);
      if (static_cast<u64>(guid) != 0) {
        HORSE_LOG_CORE_INFO("MaterialRegistry: Resolved friendly name '{0}' to "
                            "GUID {1}",
                            nameOrGuid, static_cast<u64>(guid));
      }
    }

    if (static_cast<u64>(guid) != 0) {
      auto &am = AssetManager::Get();
      std::filesystem::path path = am.GetFileSystemPath(guid);
      HORSE_LOG_CORE_INFO("MaterialRegistry: Resolving GUID {0} -> Path: {1}",
                          static_cast<u64>(guid), path.string());
      if (!path.empty()) {
        auto material = LoadMaterial(path.string());
        if (material) {
          m_Materials[nameOrGuid] = material;
          return material;
        } else {
          HORSE_LOG_CORE_ERROR(
              "MaterialRegistry: Failed to load material at {0}",
              path.string());
        }
      } else {
        HORSE_LOG_CORE_WARN("MaterialRegistry: No path found for GUID {0}",
                            static_cast<u64>(guid));
      }
    }
  } catch (...) {
  }

  return m_Materials["Default"];
}

std::shared_ptr<MaterialInstance>
MaterialRegistry::CreateMaterial(const std::string &name) {
  if (m_Materials.find(name) != m_Materials.end()) {
    HORSE_LOG_CORE_WARN(
        "Material '{}' already exists, returning existing instance", name);
    return m_Materials[name];
  }

  auto newMat = std::make_shared<MaterialInstance>(name);
  // Default values
  newMat->SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
  newMat->SetFloat("Roughness", 0.5f);
  newMat->SetFloat("Metalness", 0.0f);

  m_Materials[name] = newMat;
  return newMat;
}

std::shared_ptr<MaterialInstance>
MaterialRegistry::LoadMaterial(const std::string &filepath) {
  auto material = std::make_shared<MaterialInstance>("Temp");
  if (MaterialSerializer::Deserialize(filepath, *material)) {
    material->SetFilePath(filepath);

    // Check if material with this name already exists
    std::string name = material->GetName();
    if (m_Materials.find(name) != m_Materials.end()) {
      // If it exists but path is different, maybe rename?
      // For now, we overwrite or just log warning.
      // Let's ensure unique names in a real system, but here we overwrite.
      HORSE_LOG_CORE_WARN("Reloading material: {}", name);
    }
    m_Materials[name] = material;
    return material;
  }
  return nullptr;
}

void MaterialRegistry::LoadMaterialsFromDirectory(
    const std::string &directory) {
  if (!std::filesystem::exists(directory))
    return;

  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".horsemat") {
      LoadMaterial(entry.path().string());
    }
  }
}

} // namespace Horse

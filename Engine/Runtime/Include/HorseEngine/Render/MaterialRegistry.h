#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Render/Material.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Horse {

class HORSE_API MaterialRegistry {
public:
  static MaterialRegistry &Get();

  std::shared_ptr<MaterialInstance> GetMaterial(const std::string &name);
  std::shared_ptr<MaterialInstance> CreateMaterial(const std::string &name);

  // Load a single material file
  std::shared_ptr<MaterialInstance> LoadMaterial(const std::string &filepath);
  // Scan a directory recursively for .horsemat files
  void LoadMaterialsFromDirectory(const std::string &directory);

  const std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> &
  GetMaterials() const {
    return m_Materials;
  }

private:
  MaterialRegistry();
  ~MaterialRegistry() = default;

  std::unordered_map<std::string, std::shared_ptr<MaterialInstance>>
      m_Materials;
};

} // namespace Horse

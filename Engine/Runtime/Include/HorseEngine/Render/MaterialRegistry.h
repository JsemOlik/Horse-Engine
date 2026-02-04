#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Render/Material.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Horse {

class MaterialRegistry {
public:
  static MaterialRegistry &Get();

  std::shared_ptr<Material> GetMaterial(const std::string &name);
  std::shared_ptr<Material> CreateMaterial(const std::string &name);

private:
  MaterialRegistry();
  ~MaterialRegistry() = default;

  std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
};

} // namespace Horse

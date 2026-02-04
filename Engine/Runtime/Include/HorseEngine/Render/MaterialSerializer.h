#pragma once

#include "HorseEngine/Render/Material.h"
#include <string>

namespace Horse {

class MaterialSerializer {
public:
  static bool Serialize(const Material &material, const std::string &filepath);
  static bool Deserialize(const std::string &filepath, Material &outMaterial);
};

} // namespace Horse

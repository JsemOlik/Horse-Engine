#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Render/Material.h"
#include <string>

namespace Horse {

class HORSE_API MaterialSerializer {
public:
  static bool Serialize(const MaterialInstance &material,
                        const std::string &filepath);
  static bool Deserialize(const std::string &filepath,
                          MaterialInstance &outMaterial);
};

} // namespace Horse

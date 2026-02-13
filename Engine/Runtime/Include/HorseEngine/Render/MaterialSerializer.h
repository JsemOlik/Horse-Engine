#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Render/Material.h"
#include <string>

namespace Horse {

struct MaterialCookedHeader {
  char Magic[4] = {'H', 'M', 'A', 'T'};
  uint32_t Version = 1;
  uint32_t PropertyCount = 0;
  uint32_t TextureCount = 0;
};

class HORSE_API MaterialSerializer {
public:
  static bool Serialize(const MaterialInstance &material,
                        const std::string &filepath);
  static bool Deserialize(const std::string &filepath,
                          MaterialInstance &outMaterial);
};

} // namespace Horse

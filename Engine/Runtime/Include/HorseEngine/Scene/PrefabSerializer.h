#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Prefab.h"
#include <memory>
#include <string>

namespace Horse {

class HORSE_API PrefabSerializer {
public:
  static bool SerializeToJSON(const Prefab *prefab,
                              const std::string &filepath);
  static std::shared_ptr<Prefab>
  DeserializeFromJSON(const std::string &filepath);

  // Helper to create a prefab from an existing entity
  static std::shared_ptr<Prefab> CreateFromEntity(Entity entity,
                                                  const std::string &name);
};

} // namespace Horse

#pragma once
#include "AssetCooker.h"
#include <map>
#include <memory>

namespace Horse {

class CookerRegistry {
public:
  static void RegisterCooker(AssetType type,
                             std::unique_ptr<AssetCooker> cooker) {
    GetInternalRegistry()[type] = std::move(cooker);
  }

  static AssetCooker *GetCooker(AssetType type) {
    auto &registry = GetInternalRegistry();
    if (registry.find(type) != registry.end()) {
      return registry[type].get();
    }
    return nullptr;
  }

private:
  static std::map<AssetType, std::unique_ptr<AssetCooker>> &
  GetInternalRegistry() {
    static std::map<AssetType, std::unique_ptr<AssetCooker>> s_Registry;
    return s_Registry;
  }
};

} // namespace Horse

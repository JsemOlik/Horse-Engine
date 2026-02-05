#pragma once
#include "HorseEngine/Asset/Asset.h"
#include <filesystem>
#include <string>
#include <vector>


namespace Horse {

struct CookerContext {
  std::filesystem::path AssetsDir;
  std::filesystem::path OutputDir;
  std::string Platform;
};

class AssetCooker {
public:
  virtual ~AssetCooker() = default;

  virtual bool Cook(const std::filesystem::path &sourcePath,
                    const AssetMetadata &metadata,
                    const CookerContext &context) = 0;
  virtual std::string GetCookedExtension() const = 0;
};

} // namespace Horse

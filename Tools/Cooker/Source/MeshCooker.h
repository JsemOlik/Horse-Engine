#pragma once
#include "AssetCooker.h"

namespace Horse {

class MeshCooker : public AssetCooker {
public:
  virtual bool Cook(const std::filesystem::path &sourcePath,
                    const AssetMetadata &metadata,
                    const CookerContext &context) override;
  virtual std::string GetCookedExtension() const override {
    return ".horsemesh";
  }
};

} // namespace Horse

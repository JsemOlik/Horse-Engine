#pragma once
#include "AssetCooker.h"

namespace Horse {

class TextureCooker : public AssetCooker {
public:
  virtual bool Cook(const std::filesystem::path &sourcePath,
                    const AssetMetadata &metadata,
                    const CookerContext &context) override;
  virtual std::string GetCookedExtension() const override {
    return ".horsetex";
  }
};

} // namespace Horse

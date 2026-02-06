#pragma once
#include "AssetCooker.h"

namespace Horse {

class ScriptCooker : public AssetCooker {
public:
  bool Cook(const std::filesystem::path &sourcePath,
            const AssetMetadata &metadata,
            const CookerContext &context) override;
  std::string GetCookedExtension() const override { return ".lua"; }
};

} // namespace Horse

#pragma once

#include "HorseEngine/Asset/AssetImporter.h"

namespace Horse {

class HORSE_API MeshImporter : public AssetImporter {
public:
  bool Import(const std::filesystem::path &path) override;
  bool CanImport(const std::filesystem::path &path) const override;
};

} // namespace Horse

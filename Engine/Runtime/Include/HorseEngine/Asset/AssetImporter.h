#pragma once

#include "HorseEngine/Asset/Asset.h"
#include "HorseEngine/Core.h"
#include <filesystem>


namespace Horse {

class HORSE_API AssetImporter {
public:
  virtual ~AssetImporter() = default;

  virtual bool Import(const std::filesystem::path &path) = 0;
  virtual bool CanImport(const std::filesystem::path &path) const = 0;
};

} // namespace Horse

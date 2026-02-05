#pragma once

#include "HorseEngine/Scene/UUID.h"
#include <filesystem>
#include <string>

namespace Horse {

enum class AssetType {
  None = 0,
  Texture,
  Mesh,
  Material,
  Scene,
  Script,
  Prefab
};

struct AssetMetadata {
  UUID Handle;
  AssetType Type = AssetType::None;
  std::filesystem::path FilePath; // Relative to Assets directory

  bool IsValid() const { return Handle != 0 && Type != AssetType::None; }
};

inline std::string AssetTypeToString(AssetType type) {
  switch (type) {
  case AssetType::Texture:
    return "Texture";
  case AssetType::Mesh:
    return "Mesh";
  case AssetType::Material:
    return "Material";
  case AssetType::Scene:
    return "Scene";
  case AssetType::Script:
    return "Script";
  case AssetType::Prefab:
    return "Prefab";
  case AssetType::None:
    return "None";
  }
  return "None";
}

inline AssetType AssetTypeFromString(const std::string &assetType) {
  if (assetType == "Texture")
    return AssetType::Texture;
  if (assetType == "Mesh")
    return AssetType::Mesh;
  if (assetType == "Material")
    return AssetType::Material;
  if (assetType == "Scene")
    return AssetType::Scene;
  if (assetType == "Script")
    return AssetType::Script;
  if (assetType == "Prefab")
    return AssetType::Prefab;
  return AssetType::None;
}

} // namespace Horse

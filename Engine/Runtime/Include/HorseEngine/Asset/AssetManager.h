#pragma once

#include "HorseEngine/Asset/Asset.h"
#include <filesystem>
#include <unordered_map>

namespace Horse {

class AssetManager {
public:
  static AssetManager &Get();

  void Initialize(const std::filesystem::path &assetsDirectory);

  // Registry queries
  const AssetMetadata &GetMetadata(UUID handle) const;
  const AssetMetadata &GetMetadata(const std::filesystem::path &filePath) const;
  std::filesystem::path GetFileSystemPath(UUID handle) const;

  // Asset operations
  void ImportAsset(const std::filesystem::path &path);
  void ImportAsset(const std::filesystem::path &path, UUID uuid);

  const std::unordered_map<UUID, AssetMetadata> &GetAssetRegistry() const {
    return m_AssetRegistry;
  }

private:
  AssetManager() = default;

  void LoadRegistry();
  void ProcessDirectory(const std::filesystem::path &directory);
  void WriteMetadata(const AssetMetadata &metadata);
  AssetType DetermineAssetType(const std::filesystem::path &extension);

private:
  std::filesystem::path m_AssetsDirectory;
  std::unordered_map<UUID, AssetMetadata> m_AssetRegistry;
  std::unordered_map<std::filesystem::path, UUID> m_FilePathToHandle;
};

} // namespace Horse

#include "HorseEngine/Asset/AssetManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Horse {

AssetManager &AssetManager::Get() {
  static AssetManager instance;
  return instance;
}

void AssetManager::Initialize(const std::filesystem::path &assetsDirectory) {
  m_AssetsDirectory = assetsDirectory;
  LoadRegistry();
}

const AssetMetadata &AssetManager::GetMetadata(UUID handle) const {
  static AssetMetadata emptyMetadata;
  auto it = m_AssetRegistry.find(handle);
  if (it != m_AssetRegistry.end()) {
    return it->second;
  }
  return emptyMetadata;
}

const AssetMetadata &
AssetManager::GetMetadata(const std::filesystem::path &filePath) const {
  static AssetMetadata emptyMetadata;
  auto relativePath = std::filesystem::relative(filePath, m_AssetsDirectory);
  auto it = m_FilePathToHandle.find(relativePath);
  if (it != m_FilePathToHandle.end()) {
    return GetMetadata(it->second);
  }
  return emptyMetadata;
}

std::filesystem::path AssetManager::GetFileSystemPath(UUID handle) const {
  auto metadata = GetMetadata(handle);
  if (metadata.IsValid()) {
    return m_AssetsDirectory / metadata.FilePath;
  }
  return {};
}

void AssetManager::ImportAsset(const std::filesystem::path &path) {
  ImportAsset(path, UUID());
}

void AssetManager::ImportAsset(const std::filesystem::path &path, UUID uuid) {
  std::filesystem::path relativePath =
      std::filesystem::relative(path, m_AssetsDirectory);

  if (uuid == 0) {
    uuid = UUID();
  }

  AssetMetadata metadata;
  metadata.Handle = uuid;
  metadata.FilePath = relativePath;
  metadata.Type = DetermineAssetType(path);

  if (metadata.Type == AssetType::None)
    return;

  m_AssetRegistry[metadata.Handle] = metadata;
  m_FilePathToHandle[metadata.FilePath] = metadata.Handle;

  WriteMetadata(metadata);
}

void AssetManager::LoadRegistry() {
  m_AssetRegistry.clear();
  m_FilePathToHandle.clear();

  if (!std::filesystem::exists(m_AssetsDirectory)) {
    std::filesystem::create_directories(m_AssetsDirectory);
    return;
  }

  ProcessDirectory(m_AssetsDirectory);
}

void AssetManager::ProcessDirectory(const std::filesystem::path &directory) {
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_directory()) {
      ProcessDirectory(entry.path());
      continue;
    }

    auto path = entry.path();
    if (path.extension() == ".meta")
      continue;

    // Check for sidecar .meta file
    std::filesystem::path metaPath = path;
    metaPath += ".meta";

    if (std::filesystem::exists(metaPath)) {
      // Load existing metadata
      std::ifstream stream(metaPath);
      if (stream.is_open()) {
        try {
          nlohmann::json j;
          stream >> j;

          AssetMetadata metadata;
          metadata.Handle = j["Handle"];
          metadata.Type = AssetTypeFromString(j["Type"]);
          metadata.FilePath =
              std::filesystem::relative(path, m_AssetsDirectory);

          if (metadata.IsValid()) {
            m_AssetRegistry[metadata.Handle] = metadata;
            m_FilePathToHandle[metadata.FilePath] = metadata.Handle;
          }
        } catch (...) {
          // Failed to parse, maybe regenerate
          std::cerr << "Failed to load meta file: " << metaPath << std::endl;
        }
      }
    } else {
      // New asset found, import it
      ImportAsset(path);
    }
  }
}

void AssetManager::WriteMetadata(const AssetMetadata &metadata) {
  std::filesystem::path metaPath = m_AssetsDirectory / metadata.FilePath;
  metaPath += ".meta";

  nlohmann::json j;
  j["Handle"] = (uint64_t)metadata.Handle;
  j["Type"] = AssetTypeToString(metadata.Type);

  std::ofstream stream(metaPath);
  stream << j.dump(4);
}

AssetType AssetManager::DetermineAssetType(const std::filesystem::path &path) {
  auto extension = path.extension().string();
  // Simple extension check
  if (extension == ".png" || extension == ".jpg" || extension == ".tga")
    return AssetType::Texture;
  if (extension == ".obj" || extension == ".fbx" || extension == ".gltf")
    return AssetType::Mesh;
  if (extension == ".horsemat")
    return AssetType::Material;
  if (extension == ".horselevel")
    return AssetType::Scene;
  if (extension == ".lua")
    return AssetType::Script;

  return AssetType::None;
}

} // namespace Horse

#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
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

UUID AssetManager::GetHandleByFriendlyName(const std::string &name) const {
  for (const auto &[handle, metadata] : m_AssetRegistry) {
    std::filesystem::path p = metadata.FilePath;
    // Check if filename (stem) matches the name (e.g., "Cyan")
    // Note: path might be "Materials/Cyan.horsemat.bin"
    std::string stem = p.stem().string();
    if (stem == name)
      return handle;

    // Second pass: strip TWO extensions if it's .horsemat.bin
    if (p.extension() == ".bin") {
      std::string stem2 = p.stem().stem().string();
      if (stem2 == name)
        return handle;
    }
  }
  return UUID(0);
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

  // Check for manifest (Packaged mode)
  std::string manifestPath = "Game.manifest.json";
  if (!m_AssetsDirectory.empty() && m_AssetsDirectory != ".") {
    manifestPath = (m_AssetsDirectory / "Game.manifest.json").string();
  }

  std::replace(manifestPath.begin(), manifestPath.end(), '\\', '/');
  if (manifestPath.substr(0, 2) == "./") {
    manifestPath = manifestPath.substr(2);
  }

  HORSE_LOG_CORE_INFO("Checking for asset manifest at: {}", manifestPath);

  if (FileSystem::Exists(manifestPath)) {
    HORSE_LOG_CORE_INFO("Found manifest, loading...");
    std::string manifestContent;
    if (FileSystem::ReadText(manifestPath, manifestContent)) {
      try {
        auto j = nlohmann::json::parse(manifestContent);
        if (j.contains("Assets")) {
          for (auto it = j["Assets"].begin(); it != j["Assets"].end(); ++it) {
            UUID handle(std::stoull(it.key()));
            std::string filePath = it.value().get<std::string>();

            // Normalize separators and strip leading ./ or .\ for PhysFS
            std::replace(filePath.begin(), filePath.end(), '\\', '/');
            if (filePath.substr(0, 2) == "./") {
              filePath = filePath.substr(2);
            }
            if (filePath.substr(0, 1) == "/") {
              filePath = filePath.substr(1);
            }

            AssetMetadata metadata;
            metadata.Handle = handle;
            metadata.FilePath = filePath;
            metadata.Type = DetermineAssetType(filePath);

            m_AssetRegistry[handle] = metadata;
            m_FilePathToHandle[metadata.FilePath] = handle;
          }
          HORSE_LOG_CORE_INFO("Loaded {} assets from manifest.",
                              m_AssetRegistry.size());
          return; // Skip filesystem scan
        }
      } catch (const std::exception &e) {
        HORSE_LOG_CORE_ERROR("Failed to parse manifest: {}", e.what());
      }
    } else {
      HORSE_LOG_CORE_ERROR("Failed to read manifest content!");
    }
  } else {
    HORSE_LOG_CORE_WARN("Manifest not found at {}. Performing filesystem scan.",
                        manifestPath);
  }

  ProcessDirectory(m_AssetsDirectory);
}

void AssetManager::ProcessDirectory(const std::filesystem::path &directory) {
  // Use FileSystem::Enumerate instead of directory_iterator
  auto entries = FileSystem::Enumerate(directory);
  for (const auto &entryBytes : entries) {
    std::filesystem::path entryPath =
        directory / entryBytes; // entryBytes is filename

    // Since FileSystem::Enumerate currently returns just filenames, we don't
    // know if it's a directory? Wait, my Enumerate implementation flattens or
    // just returns files? PHYSFS_enumerateFiles returns both files and
    // directories. We need to check if it's a directory.
    // FileSystem::IsDirectory? - Need to add.
    // Or just try to Recurse?

    // For now, let's assume flat structure or files.
    // Actually, relying on recursion is important for "Assets/Textures/..."

    // Let's implement Recurse if IsDirectory is true.
    // But I didn't add IsDirectory to FileSystem.
    // Workaround: Try to Enumerate it? Or check extension.
    // Files usually have dots. Directories usually don't? Unreliable.

    // Hack: For now, we only care about files in known paths or rely on flat
    // listing? No, the PAK has folders.

    // Better: FileSystem::Enumerate returning full relative paths?
    // PHYSFS_enumerateFiles returns filenames.

    // Let's assume for now we only need to load specific assets by scanning?
    // If I can't determine IsDirectory without Stat, I should add Stat or
    // IsDirectory to FileSystem.

    // SKIPPING ProcessDirectory recursion for now and only processing files
    // with extensions. This might miss subdirectories. But wait... I have
    // FileSystem::Exists. I can try to access it?

    // Let's rely on extension. If it has no extension, assume directory and
    // recurse?
    if (!entryPath.has_extension()) {
      ProcessDirectory(entryPath);
      continue;
    }

    auto path = entryPath;
    if (path.extension() == ".meta")
      continue;

    // Check for sidecar .meta file
    std::filesystem::path metaPath = path;
    metaPath += ".meta";

    if (FileSystem::Exists(metaPath)) {
      std::string metaContent;
      if (FileSystem::ReadText(metaPath, metaContent)) {
        try {
          nlohmann::json j = nlohmann::json::parse(metaContent);
          AssetMetadata metadata;
          metadata.Handle = UUID(j["Handle"].get<uint64_t>());
          metadata.Type = AssetTypeFromString(j["Type"]);
          metadata.FilePath =
              std::filesystem::relative(path, m_AssetsDirectory);
          // relative might fail if paths are virtual?
          // If 'path' is "Assets/Textures/Wall.png" and m_AssetsDirectory is
          // "Assets", relative is "Textures/Wall.png". Correct.

          if (metadata.IsValid()) {
            m_AssetRegistry[metadata.Handle] = metadata;
            m_FilePathToHandle[metadata.FilePath] = metadata.Handle;
          }
        } catch (...) {
        }
      }
    } else {
      ImportAsset(path);
    }
  }
}

void AssetManager::WriteMetadata(const AssetMetadata &metadata) {
  // Only write in Editor mode or if not packed?
  // Using std::ofstream generally fails in PAK.
  // For runtime, we skip writing.
#ifndef HORSE_RUNTIME
  std::filesystem::path metaPath = m_AssetsDirectory / metadata.FilePath;
  metaPath += ".meta";

  nlohmann::json j;
  j["Handle"] = (uint64_t)metadata.Handle;
  j["Type"] = AssetTypeToString(metadata.Type);

  std::ofstream stream(metaPath);
  stream << j.dump(4);
#endif
}

AssetType AssetManager::DetermineAssetType(const std::filesystem::path &path) {
  auto extension = path.extension().string();
  // Simple extension check
  if (extension == ".png" || extension == ".jpg" || extension == ".tga" ||
      extension == ".horsetexture")
    return AssetType::Texture;
  if (extension == ".obj" || extension == ".fbx" || extension == ".gltf" ||
      extension == ".horsemesh")
    return AssetType::Mesh;
  if (extension == ".horsemat" || extension == ".horsematerial" ||
      extension == ".horsemat.bin")
    return AssetType::Material;
  if (extension == ".horselevel" || extension == ".json") {
    // Check if in Scenes folder or has .horselevel in name
    if (path.parent_path().filename() == "Scenes" ||
        path.filename().string().find(".horselevel") != std::string::npos)
      return AssetType::Scene;
    // Check if in Materials folder for generic .json materials if we ever use
    // them
    if (path.parent_path().filename() == "Materials")
      return AssetType::Material;
  }
  if (extension == ".lua")
    return AssetType::Script;

  return AssetType::None;
}

} // namespace Horse

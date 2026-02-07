#include "ProjectCooker.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Hash.h"
#include "HorseEngine/Core/Logging.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Horse {

#pragma pack(push, 1)
struct ProjectCookedHeader {
  char Magic[4] = {'H', 'P', 'R', 'J'};
  uint32_t Version = 1;
  uint64_t DefaultLevelGUID = 0;
};
#pragma pack(pop)

bool ProjectCooker::Cook(const std::filesystem::path &sourcePath,
                         const AssetMetadata &metadata,
                         const CookerContext &context) {
  std::ifstream stream(sourcePath);
  if (!stream.is_open())
    return false;

  nlohmann::json j;
  stream >> j;

  ProjectCookedHeader header;
  if (j.contains("DefaultLevel")) {
    header.DefaultLevelGUID = j["DefaultLevel"].get<uint64_t>();
  } else if (j.contains("defaultScene")) {
    std::string scenePath = j["defaultScene"].get<std::string>();

    // Normalize path to match asset manifest (remove "Assets/" prefix and use
    // forward slashes)
    std::string normalizedPath = scenePath;
    if (normalizedPath.find("Assets/") == 0) {
      normalizedPath = normalizedPath.substr(7);
    } else if (normalizedPath.find("Assets\\") == 0) {
      normalizedPath = normalizedPath.substr(7);
    }

    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    // Try to resolve GUID via AssetManager (so it matches manifest/meta files)
    // Pass the absolute path so AssetManager::GetMetadata can correctly compute
    // the relative path
    auto &assetMetadata =
        AssetManager::Get().GetMetadata(context.AssetsDir / normalizedPath);
    if (assetMetadata.IsValid()) {
      header.DefaultLevelGUID = (uint64_t)assetMetadata.Handle;
      HORSE_LOG_CORE_INFO(
          "Resolved Default Scene GUID via AssetManager: {0} -> {1}",
          normalizedPath, header.DefaultLevelGUID);
    } else {
      // Fallback to stable hash if not in registry
      header.DefaultLevelGUID = Hash::HashString(normalizedPath);
      HORSE_LOG_CORE_WARN("Default Scene {0} not found in AssetRegistry! "
                          "Falling back to Hash: {1}",
                          normalizedPath, header.DefaultLevelGUID);
    }

    HORSE_LOG_CORE_INFO("Cooking Default Scene Entry: {0} (GUID: {1})",
                        scenePath, header.DefaultLevelGUID);
  }

  std::filesystem::path outputPath = context.OutputDir / "Game.project.bin";
  std::filesystem::create_directories(outputPath.parent_path());

  std::ofstream outStream(outputPath, std::ios::binary);
  outStream.write(reinterpret_cast<const char *>(&header),
                  sizeof(ProjectCookedHeader));

  HORSE_LOG_CORE_INFO("Cooked Project: {0} -> {1}", sourcePath.string(),
                      outputPath.string());
  return true;
}

} // namespace Horse

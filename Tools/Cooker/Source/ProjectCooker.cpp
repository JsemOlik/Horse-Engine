#include "ProjectCooker.h"
#include "HorseEngine/Core/Logging.h"
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

    // Store hashed filename as GUID to match manifest
    header.DefaultLevelGUID = std::hash<std::string>{}(normalizedPath);
    HORSE_LOG_CORE_INFO(
        "Cooking Default Scene: {0} (Normalized: {1}, GUID: {2})", scenePath,
        normalizedPath, header.DefaultLevelGUID);
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

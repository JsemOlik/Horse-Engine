#include "ProjectCooker.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Logging.h"
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>


namespace Horse {

struct ProjectCookedHeader {
  char Magic[4] = {'H', 'P', 'R', 'J'};
  uint32_t Version = 1;
  char Name[128] = {0};
  uint64_t DefaultLevelGUID = 0;
};

bool ProjectCooker::Cook(const std::filesystem::path &sourcePath,
                         const AssetMetadata &metadata,
                         const CookerContext &context) {
  std::ifstream stream(sourcePath);
  if (!stream.is_open())
    return false;

  nlohmann::json j;
  stream >> j;

  ProjectCookedHeader header;
  if (j.contains("name")) {
    std::string name = j["name"].get<std::string>();
    strncpy(header.Name, name.c_str(), sizeof(header.Name) - 1);
  }

  if (j.contains("DefaultLevel")) {
    header.DefaultLevelGUID = j["DefaultLevel"].get<uint64_t>();
  } else if (j.contains("defaultScene")) {
    std::string scenePathStr = j["defaultScene"].get<std::string>();
    // Resolve relative to project file, then make relative to AssetsDir
    std::filesystem::path sceneFullPath =
        sourcePath.parent_path() / scenePathStr;
    std::string relToAssets =
        std::filesystem::relative(sceneFullPath, context.AssetsDir)
            .generic_string();

    auto &meta = AssetManager::Get().GetMetadata(relToAssets);
    if (meta.IsValid()) {
      header.DefaultLevelGUID = (uint64_t)meta.Handle;
      HORSE_LOG_CORE_INFO("ProjectCooker: Resolved default scene '{}' to GUID: "
                          "{}",
                          scenePathStr, header.DefaultLevelGUID);
    } else {
      HORSE_LOG_CORE_ERROR(
          "ProjectCooker: Could not find default scene in AssetManager: {}",
          relToAssets);
      // Fallback to hashing if not found? No, better to fail or let it be 0.
      header.DefaultLevelGUID = 0;
    }
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

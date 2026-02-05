#include "CookerRegistry.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Logging.h"
#include "LevelCooker.h"
#include "MaterialCooker.h"
#include "MeshCooker.h"
#include "ProjectCooker.h"
#include "TextureCooker.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace Horse;

void PrintUsage() {
  std::cout << "Usage: HorseCooker <AssetsDir> <OutputDir> [Platform]"
            << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    PrintUsage();
    return 1;
  }

  std::filesystem::path assetsDir = std::filesystem::absolute(argv[1]);
  std::filesystem::path outputDir = std::filesystem::absolute(argv[2]);
  std::string platform = (argc > 3) ? argv[3] : "Windows";

  Logger::Initialize();
  HORSE_LOG_CORE_INFO("HorseCooker starting...");
  HORSE_LOG_CORE_INFO("Assets Directory: {0}", assetsDir.string());
  HORSE_LOG_CORE_INFO("Output Directory: {0}", outputDir.string());
  HORSE_LOG_CORE_INFO("Target Platform: {0}", platform);

  if (!std::filesystem::exists(assetsDir)) {
    HORSE_LOG_CORE_ERROR("Assets directory does not exist!");
    return 1;
  }

  if (!std::filesystem::exists(outputDir)) {
    std::filesystem::create_directories(outputDir);
  }

  // Initialize AssetManager
  AssetManager::Get().Initialize(assetsDir);

  // Register Cookers
  CookerRegistry::RegisterCooker(AssetType::Texture,
                                 std::make_unique<TextureCooker>());
  CookerRegistry::RegisterCooker(AssetType::Mesh,
                                 std::make_unique<MeshCooker>());
  CookerRegistry::RegisterCooker(AssetType::Material,
                                 std::make_unique<MaterialCooker>());
  CookerRegistry::RegisterCooker(AssetType::Scene,
                                 std::make_unique<LevelCooker>());

  CookerContext context;
  context.AssetsDir = assetsDir;
  context.OutputDir = outputDir;
  context.Platform = platform;

  nlohmann::json manifest;
  manifest["Assets"] = nlohmann::json::object();

  const auto &registry = AssetManager::Get().GetAssetRegistry();
  HORSE_LOG_CORE_INFO("Processing {0} assets...", registry.size());

  for (const auto &[handle, metadata] : registry) {
    AssetCooker *cooker = CookerRegistry::GetCooker(metadata.Type);
    if (cooker) {
      std::filesystem::path sourcePath =
          AssetManager::Get().GetFileSystemPath(handle);
      if (cooker->Cook(sourcePath, metadata, context)) {
        std::filesystem::path cookedRelPath = metadata.FilePath;
        cookedRelPath.replace_extension(cooker->GetCookedExtension());
        manifest["Assets"][std::to_string((uint64_t)handle)] =
            cookedRelPath.string();
      }
    }
  }

  // Cook Project File
  std::filesystem::path projectFile;
  for (const auto &entry :
       std::filesystem::directory_iterator(assetsDir.parent_path())) {
    if (entry.path().extension() == ".horseproject") {
      projectFile = entry.path();
      break;
    }
  }

  if (!projectFile.empty()) {
    ProjectCooker projCooker;
    AssetMetadata projMeta;
    projMeta.FilePath = projectFile.filename();
    projCooker.Cook(projectFile, projMeta, context);
  }

  // Save Manifest
  std::ofstream manifestStream((outputDir / "Game.manifest.json").string());
  manifestStream << manifest.dump(4);

  HORSE_LOG_CORE_INFO("Cooking finished successfully.");
  return 0;
}

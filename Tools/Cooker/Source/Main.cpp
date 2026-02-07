#include "CookerRegistry.h"
#include "HorseEngine/Asset/Asset.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Hash.h"
#include "HorseEngine/Core/Logging.h"
#include "LevelCooker.h"
#include "MaterialCooker.h"
#include "MeshCooker.h"
#include "ProjectCooker.h"
#include "ScriptCooker.h"
#include "TextureCooker.h"


#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <windows.h>

using namespace Horse;

void PrintUsage() {
  std::cout << "Usage: HorseCooker <AssetsDir> <OutputDir> [Platform]"
            << std::endl;
}

AssetType GetTypeFromExtension(const std::filesystem::path &path) {
  std::string ext = path.extension().string();
  for (auto &c : ext)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
      ext == ".bmp")
    return AssetType::Texture;
  if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb")
    return AssetType::Mesh;
  if (ext == ".horsemat" || ext == ".mat")
    return AssetType::Material;
  if (ext == ".lua")
    return AssetType::Script;

  std::string filename = path.filename().string();
  for (auto &c : filename)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  if (filename.find(".horselevel") != std::string::npos || ext == ".json") {
    // Check if parent directory is "Scenes" or filename contains it
    std::string folder = path.parent_path().filename().string();
    if (folder == "Scenes" || filename.find(".horselevel") != std::string::npos)
      return AssetType::Scene;
    if (folder == "Materials")
      return AssetType::Material;
    return AssetType::None;
  }
  return AssetType::None;
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
  CookerRegistry::RegisterCooker(AssetType::Script,
                                 std::make_unique<ScriptCooker>());

  CookerContext context;
  context.AssetsDir = assetsDir;
  context.OutputDir = outputDir;
  context.Platform = platform;

  nlohmann::json manifest;
  manifest["Assets"] = nlohmann::json::object();

  const auto &registry = AssetManager::Get().GetAssetRegistry();
  std::unordered_map<UUID, AssetMetadata> cookingRegistry = registry;

  // Auto-discovery if registry is empty
  if (cookingRegistry.empty()) {
    HORSE_LOG_CORE_INFO("AssetRegistry.json not found or empty. Scanning "
                        "filesystem for assets...");
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(assetsDir)) {
      if (entry.is_regular_file()) {
        HORSE_LOG_CORE_INFO("Scanning: {0}", entry.path().string());
        if (entry.path().filename() == "AssetRegistry.json")
          continue;
        if (entry.path().extension() == ".meta")
          continue;

        AssetType type = GetTypeFromExtension(entry.path());
        if (type != AssetType::None) {
          AssetMetadata meta;
          // Use hashed relative path as stable GUID
          std::string relPathStr =
              std::filesystem::relative(entry.path(), assetsDir).string();
          std::replace(relPathStr.begin(), relPathStr.end(), '\\', '/');
          meta.Handle = UUID(Hash::HashString(relPathStr));
          meta.Type = type;
          meta.FilePath = relPathStr;
          cookingRegistry[meta.Handle] = meta;
          HORSE_LOG_CORE_INFO("  -> Identified as {0}",
                              AssetTypeToString(type));
        } else {
          HORSE_LOG_CORE_INFO("  -> Skipped (Unknown type)");
        }
      }
    }
  }

  HORSE_LOG_CORE_INFO("Processing {0} assets...", cookingRegistry.size());

  for (const auto &[handle, metadata] : cookingRegistry) {
    AssetCooker *cooker = CookerRegistry::GetCooker(metadata.Type);
    if (cooker) {
      std::filesystem::path sourcePath = assetsDir / metadata.FilePath;
      if (cooker->Cook(sourcePath, metadata, context)) {
        std::filesystem::path cookedRelPath = metadata.FilePath;
        if (cookedRelPath.extension() == ".json") {
          cookedRelPath.replace_extension("");
        }
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
    if (entry.path().extension() == ".horseproject" ||
        entry.path().string().ends_with(".horseproject.json")) {
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

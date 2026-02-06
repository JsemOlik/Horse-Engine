#include "ScriptCooker.h"
#include "HorseEngine/Core/Logging.h"
#include <fstream>

namespace Horse {

bool ScriptCooker::Cook(const std::filesystem::path &sourcePath,
                        const AssetMetadata &metadata,
                        const CookerContext &context) {
  std::filesystem::path outputPath =
      context.OutputDir / "Assets" / metadata.FilePath;

  // Ensure output directory exists
  std::filesystem::create_directories(outputPath.parent_path());

  try {
    std::filesystem::copy_file(
        sourcePath, outputPath,
        std::filesystem::copy_options::overwrite_existing);
    HORSE_LOG_CORE_INFO("Cooked Script: {0}", sourcePath.string());
    return true;
  } catch (const std::exception &e) {
    HORSE_LOG_CORE_ERROR("Failed to cook script: {0}. Error: {1}",
                         sourcePath.string(), e.what());
    return false;
  }
}

} // namespace Horse

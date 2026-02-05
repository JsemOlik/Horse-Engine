#include "MaterialCooker.h"
#include "HorseEngine/Core/Logging.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace Horse {

struct MaterialCookedHeader {
  char Magic[4] = {'H', 'M', 'A', 'T'};
  uint32_t Version = 1;
  uint32_t PropertyCount = 0;
  uint32_t TextureCount = 0;
};

bool MaterialCooker::Cook(const std::filesystem::path &sourcePath,
                          const AssetMetadata &metadata,
                          const CookerContext &context) {
  std::ifstream stream(sourcePath);
  if (!stream.is_open())
    return false;

  nlohmann::json j;
  stream >> j;

  MaterialCookedHeader header;
  // logic to extract counts...

  std::filesystem::path outputPath = context.OutputDir / metadata.FilePath;
  outputPath.replace_extension(GetCookedExtension());
  std::filesystem::create_directories(outputPath.parent_path());

  std::ofstream outStream(outputPath, std::ios::binary);
  outStream.write(reinterpret_cast<const char *>(&header),
                  sizeof(MaterialCookedHeader));

  // Write the rest of the material data (simplified for now)
  std::string jsonStr = j.dump();
  uint32_t size = static_cast<uint32_t>(jsonStr.size());
  outStream.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
  outStream.write(jsonStr.data(), size);

  HORSE_LOG_CORE_INFO("Cooked Material: {0} -> {1}", sourcePath.string(),
                      outputPath.string());
  return true;
}

} // namespace Horse

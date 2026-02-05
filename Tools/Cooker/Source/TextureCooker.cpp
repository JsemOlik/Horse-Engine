#include "TextureCooker.h"
#include "HorseEngine/Core/Logging.h"

#define STB_IMAGE_IMPLEMENTATION
#include <fstream>
#include <stb_image.h>
#include <vector>


namespace Horse {

struct TextureHeader {
  char Magic[4] = {'H', 'T', 'E', 'X'};
  uint32_t Version = 1;
  uint32_t Width = 0;
  uint32_t Height = 0;
  uint32_t Format = 0; // 0 = RGBA8
  uint32_t MipCount = 1;
  uint32_t DataSize = 0;
};

bool TextureCooker::Cook(const std::filesystem::path &sourcePath,
                         const AssetMetadata &metadata,
                         const CookerContext &context) {
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load(sourcePath.string().c_str(), &width, &height, &channels, 4);

  if (!data) {
    HORSE_LOG_CORE_ERROR("Failed to load texture for cooking: {0}",
                         sourcePath.string());
    return false;
  }

  uint32_t dataSize = width * height * 4;

  TextureHeader header;
  header.Width = static_cast<uint32_t>(width);
  header.Height = static_cast<uint32_t>(height);
  header.DataSize = dataSize;

  std::filesystem::path outputPath = context.OutputDir / metadata.FilePath;
  outputPath.replace_extension(GetCookedExtension());

  std::filesystem::create_directories(outputPath.parent_path());

  std::ofstream stream(outputPath, std::ios::binary);
  if (!stream.is_open()) {
    HORSE_LOG_CORE_ERROR("Failed to open output file for cooking: {0}",
                         outputPath.string());
    stbi_image_free(data);
    return false;
  }

  stream.write(reinterpret_cast<const char *>(&header), sizeof(TextureHeader));
  stream.write(reinterpret_cast<const char *>(data), dataSize);

  stbi_image_free(data);
  HORSE_LOG_CORE_INFO("Cooked texture: {0} -> {1}", sourcePath.string(),
                      outputPath.string());

  return true;
}

} // namespace Horse

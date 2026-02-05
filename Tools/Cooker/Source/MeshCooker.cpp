#include "MeshCooker.h"
#include "HorseEngine/Core/Logging.h"
#include <fstream>

namespace Horse {

struct MeshCookedHeader {
  char Magic[4] = {'H', 'M', 'S', 'H'};
  uint32_t Version = 1;
  uint32_t VertexCount = 0;
  uint32_t IndexCount = 0;
};

bool MeshCooker::Cook(const std::filesystem::path &sourcePath,
                      const AssetMetadata &metadata,
                      const CookerContext &context) {
  // TODO: Use Assimp to load and optimize
  // For now, let's just create a binary copy of the source or a dummy binary

  MeshCookedHeader header;

  std::filesystem::path outputPath = context.OutputDir / metadata.FilePath;
  outputPath.replace_extension(GetCookedExtension());
  std::filesystem::create_directories(outputPath.parent_path());

  std::ofstream stream(outputPath, std::ios::binary);
  if (!stream.is_open())
    return false;

  stream.write(reinterpret_cast<const char *>(&header),
               sizeof(MeshCookedHeader));

  // Dummy data for now to test the pipeline
  HORSE_LOG_CORE_INFO("Cooked Mesh (Skeleton): {0} -> {1}", sourcePath.string(),
                      outputPath.string());
  return true;
}

} // namespace Horse

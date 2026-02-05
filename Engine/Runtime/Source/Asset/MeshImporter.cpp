#include "HorseEngine/Asset/MeshImporter.h"
#include <iostream>

namespace Horse {

bool MeshImporter::Import(const std::filesystem::path &path) {
  // Logic to process mesh (e.g. validate with Assimp or convert)
  return true;
}

bool MeshImporter::CanImport(const std::filesystem::path &path) const {
  std::string ext = path.extension().string();
  return ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb";
}

} // namespace Horse

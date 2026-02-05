#include "HorseEngine/Asset/TextureImporter.h"
#include <iostream>

namespace Horse {

bool TextureImporter::Import(const std::filesystem::path &path) {
  // Logic to process texture if needed (e.g. compress or generate thumbnails)
  // For now, we trust the source file.
  return true;
}

bool TextureImporter::CanImport(const std::filesystem::path &path) const {
  std::string ext = path.extension().string();
  return ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".bmp" ||
         ext == ".hdr" || ext == ".psd";
}

} // namespace Horse

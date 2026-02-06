#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "HorseEngine/Core.h"

namespace Horse {

class HORSE_API FileSystem {
public:
  static bool Initialize(const char *argv0);
  static void Shutdown();

  static bool Mount(const std::string &archive, const std::string &mountPoint);

  // Reads entire file into buffer
  static bool ReadBytes(const std::filesystem::path &path,
                        std::vector<uint8_t> &outData);

  // Reads entire file into string
  static bool ReadText(const std::filesystem::path &path, std::string &outText);

  static bool Exists(const std::filesystem::path &path);
};

} // namespace Horse

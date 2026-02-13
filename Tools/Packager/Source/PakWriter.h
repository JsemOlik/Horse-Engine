#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Horse {

struct PakFileEntry {
  std::string Path;
  uint64_t Offset;
  uint64_t Size;
  uint64_t CompressedSize;
  uint32_t Hash; // CRC32 or similar
  uint32_t Flags;
};

class PakWriter {
public:
  PakWriter(const std::filesystem::path &outputPath);
  ~PakWriter();

  bool AddFile(const std::filesystem::path &sourcePath,
               const std::string &internalPath);
  bool Finalize();

private:
  std::filesystem::path m_OutputPath;
  std::ofstream m_OutputStream;
};

} // namespace Horse

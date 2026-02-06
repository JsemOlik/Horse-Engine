#include "PakWriter.h"
#include <iostream>
#include <vector>
#include <zlib.h>

// Simple logging
#define LOG_INFO(...)                                                          \
  printf("[Packager] " __VA_ARGS__);                                           \
  printf("\n")
#define LOG_ERROR(...)                                                         \
  printf("[Packager Error] " __VA_ARGS__);                                     \
  printf("\n")

namespace Horse {

struct PakHeader {
  char Magic[4] = {'H', 'P', 'A', 'K'};
  uint32_t Version = 1;
  uint32_t FileCount = 0;
  uint32_t Flags = 0;
  uint64_t DirOffset = 0;
};

PakWriter::PakWriter(const std::filesystem::path &outputPath)
    : m_OutputPath(outputPath) {
  m_Stream.open(outputPath, std::ios::binary);
  if (m_Stream.is_open()) {
    // Reserve space for header
    PakHeader header;
    m_Stream.write(reinterpret_cast<char *>(&header), sizeof(PakHeader));
  } else {
    LOG_ERROR("Failed to open output file: %s", outputPath.string().c_str());
  }
}

PakWriter::~PakWriter() {
  if (m_Stream.is_open()) {
    m_Stream.close();
  }
}

// Simple CRC32 implementation
uint32_t CalculateCRC32(const std::vector<uint8_t> &data) {
  uLong crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, data.data(), (uInt)data.size());
  return (uint32_t)crc;
}

void PakWriter::AddFile(const std::filesystem::path &sourcePath,
                        const std::string &internalPath) {
  if (!m_Stream.is_open())
    return;

  std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    LOG_ERROR("Failed to open source file: %s", sourcePath.string().c_str());
    return;
  }

  std::streamsize fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(fileSize);
  if (!file.read(reinterpret_cast<char *>(buffer.data()), fileSize)) {
    LOG_ERROR("Failed to read source file: %s", sourcePath.string().c_str());
    return;
  }

  // Compress
  uLongf compressedSize = compressBound((uLong)fileSize);
  std::vector<uint8_t> compressedBuffer(compressedSize);

  // Using zlib
  if (compress(compressedBuffer.data(), &compressedSize, buffer.data(),
               (uLong)fileSize) != Z_OK) {
    LOG_ERROR("Failed to compress file: %s", sourcePath.string().c_str());
    // Fallback to uncompressed? For now, just store uncompressed if fail logic
    // is needed, but here we abort entry or store uncompressed. Let's store
    // uncompressed if compression fails or is worse? For simplicity, assume
    // compression works or fail.
    return;
  }

  // Choose smaller
  bool useCompressed = compressedSize < fileSize;
  const std::vector<uint8_t> &dataToWrite =
      useCompressed ? compressedBuffer : buffer;
  uint64_t finalSize = useCompressed ? compressedSize : fileSize;

  uint64_t currentOffset = m_Stream.tellp();
  m_Stream.write(reinterpret_cast<const char *>(dataToWrite.data()), finalSize);

  PakFileEntry entry;
  entry.Path = internalPath;
  entry.Offset = currentOffset;
  entry.Size = fileSize; // Original Size
  entry.CompressedSize = finalSize;
  entry.Hash = CalculateCRC32(buffer); // Hash of original data
  entry.Flags = useCompressed ? 1 : 0; // Flag 1 = Compressed

  m_Entries.push_back(entry);
  LOG_INFO("Added: %s -> %s (Size: %llu, Comp: %llu)",
           sourcePath.string().c_str(), internalPath.c_str(), fileSize,
           finalSize);
}

bool PakWriter::Finalize() {
  if (!m_Stream.is_open())
    return false;

  // Write Directory
  m_DirectoryOffset = m_Stream.tellp();

  for (const auto &entry : m_Entries) {
    // Format: PathLength(u16), Path(chars), Offset(u64), Size(u64),
    // CompressedSize(u64), Hash(u32), Flags(u32)
    uint16_t pathLen = (uint16_t)entry.Path.length();
    m_Stream.write(reinterpret_cast<const char *>(&pathLen), sizeof(pathLen));
    m_Stream.write(entry.Path.c_str(), pathLen);
    m_Stream.write(reinterpret_cast<const char *>(&entry.Offset),
                   sizeof(entry.Offset));
    m_Stream.write(reinterpret_cast<const char *>(&entry.Size),
                   sizeof(entry.Size));
    m_Stream.write(reinterpret_cast<const char *>(&entry.CompressedSize),
                   sizeof(entry.CompressedSize));
    m_Stream.write(reinterpret_cast<const char *>(&entry.Hash),
                   sizeof(entry.Hash));
    m_Stream.write(reinterpret_cast<const char *>(&entry.Flags),
                   sizeof(entry.Flags));
  }

  // Update Header
  m_Stream.seekp(0);
  PakHeader header;
  header.FileCount = (uint32_t)m_Entries.size();
  header.DirOffset = m_DirectoryOffset;
  m_Stream.write(reinterpret_cast<char *>(&header), sizeof(PakHeader));

  LOG_INFO("Finalized PAK. Files: %zu, Dir Offset: %llu", m_Entries.size(),
           m_DirectoryOffset);
  return true;
}

} // namespace Horse

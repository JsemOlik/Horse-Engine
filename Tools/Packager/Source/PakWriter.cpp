#include "PakWriter.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <vector>
#include <zlib.h>

namespace Horse {

// ZIP Structures (packed)
#pragma pack(push, 1)

struct ZipLocalHeader {
  uint32_t signature = 0x04034b50;
  uint16_t versionNeeded = 20;
  uint16_t flags = 0;
  uint16_t compressionMethod = 8; // 8 = Deflate
  uint16_t lastModTime = 0;
  uint16_t lastModDate = 0;
  uint32_t crc32 = 0;
  uint32_t compressedSize = 0;
  uint32_t uncompressedSize = 0;
  uint16_t filenameLength = 0;
  uint16_t extraFieldLength = 0;
};

struct ZipCentralDirectoryHeader {
  uint32_t signature = 0x02014b50;
  uint16_t versionMadeBy = 20;
  uint16_t versionNeeded = 20;
  uint16_t flags = 0;
  uint16_t compressionMethod = 8;
  uint16_t lastModTime = 0;
  uint16_t lastModDate = 0;
  uint32_t crc32 = 0;
  uint32_t compressedSize = 0;
  uint32_t uncompressedSize = 0;
  uint16_t filenameLength = 0;
  uint16_t extraFieldLength = 0;
  uint16_t fileCommentLength = 0;
  uint16_t diskNumberStart = 0;
  uint16_t internalFileAttr = 0;
  uint32_t externalFileAttr = 0;
  uint32_t relativeOffsetOfLocalHeader = 0;
};

struct ZipEndOfCentralDirectory {
  uint32_t signature = 0x06054b50;
  uint16_t numberOfThisDisk = 0;
  uint16_t diskWhereCentralDirectoryStarts = 0;
  uint16_t numberOfCentralDirectoryRecordsOnThisDisk = 0;
  uint16_t totalNumberOfCentralDirectoryRecords = 0;
  uint32_t sizeOfCentralDirectory = 0;
  uint32_t offsetOfStartOfCentralDirectory = 0;
  uint16_t zipCommentLength = 0;
};

#pragma pack(pop)

struct ZipEntryInfo {
  std::string filename;
  uint32_t crc32;
  uint32_t compressedSize;
  uint32_t uncompressedSize;
  uint32_t localHeaderOffset;
};

PakWriter::PakWriter(const std::filesystem::path &outputPath)
    : m_OutputPath(outputPath) {
  m_OutputStream.open(outputPath, std::ios::binary);
  if (!m_OutputStream.is_open()) {
    spdlog::error("Failed to open output PAK file: {}", outputPath.string());
  }
}

PakWriter::~PakWriter() {
  if (m_OutputStream.is_open()) {
    m_OutputStream.close();
  }
}

static std::vector<ZipEntryInfo> s_Entries;

bool PakWriter::AddFile(const std::filesystem::path &inputPath,
                        const std::string &entryName) {
  std::ifstream file(inputPath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    spdlog::error("Failed to open input file: {}", inputPath.string());
    return false;
  }

  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  // 1. Calculate CRC32
  uLong crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()),
              (uInt)fileSize);

  // 2. Compress (Raw Deflate for ZIP)
  // Estimate size
  uLong compressedBound = compressBound((uLong)fileSize);
  std::vector<char> compressedBuffer(compressedBound);

  z_stream defstream;
  defstream.zalloc = Z_NULL;
  defstream.zfree = Z_NULL;
  defstream.opaque = Z_NULL;

  // WindowBits = -15 for raw deflate (no zlib header/trailer)
  if (deflateInit2(&defstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    spdlog::error("Failed to initialize zlib deflate");
    return false;
  }

  defstream.avail_in = (uInt)fileSize;
  defstream.next_in = (Bytef *)buffer.data();
  defstream.avail_out = (uInt)compressedBound;
  defstream.next_out = (Bytef *)compressedBuffer.data();

  deflate(&defstream, Z_FINISH);

  uLong compressedSize = defstream.total_out;
  deflateEnd(&defstream);

  // 3. Write Local Header
  ZipLocalHeader lfh;
  lfh.crc32 = (uint32_t)crc;
  lfh.compressedSize = (uint32_t)compressedSize;
  lfh.uncompressedSize = (uint32_t)fileSize;
  lfh.filenameLength = (uint16_t)entryName.length();

  uint32_t currentOffset = (uint32_t)m_OutputStream.tellp();

  m_OutputStream.write(reinterpret_cast<const char *>(&lfh),
                       sizeof(ZipLocalHeader));
  m_OutputStream.write(entryName.c_str(), entryName.length());
  m_OutputStream.write(compressedBuffer.data(), compressedSize);

  // Store info for Central Directory
  ZipEntryInfo info;
  info.filename = entryName;
  info.crc32 = lfh.crc32;
  info.compressedSize = lfh.compressedSize;
  info.uncompressedSize = lfh.uncompressedSize;
  info.localHeaderOffset = currentOffset;
  s_Entries.push_back(info);

  spdlog::info("Added file: {} (Size: {}, Compressed: {})", entryName, fileSize,
               compressedSize);
  return true;
}

bool PakWriter::Finalize() {
  uint32_t cdStartOffset = (uint32_t)m_OutputStream.tellp();

  // Write Central Directory Headers
  for (const auto &entry : s_Entries) {
    ZipCentralDirectoryHeader cdh;
    cdh.crc32 = entry.crc32;
    cdh.compressedSize = entry.compressedSize;
    cdh.uncompressedSize = entry.uncompressedSize;
    cdh.filenameLength = (uint16_t)entry.filename.length();
    cdh.relativeOffsetOfLocalHeader = entry.localHeaderOffset;

    m_OutputStream.write(reinterpret_cast<const char *>(&cdh),
                         sizeof(ZipCentralDirectoryHeader));
    m_OutputStream.write(entry.filename.c_str(), entry.filename.length());
  }

  uint32_t cdEndOffset = (uint32_t)m_OutputStream.tellp();
  uint32_t cdSize = cdEndOffset - cdStartOffset;

  // Write End of Central Directory Record
  ZipEndOfCentralDirectory eocd;
  eocd.numberOfCentralDirectoryRecordsOnThisDisk = (uint16_t)s_Entries.size();
  eocd.totalNumberOfCentralDirectoryRecords = (uint16_t)s_Entries.size();
  eocd.sizeOfCentralDirectory = cdSize;
  eocd.offsetOfStartOfCentralDirectory = cdStartOffset;

  m_OutputStream.write(reinterpret_cast<const char *>(&eocd),
                       sizeof(ZipEndOfCentralDirectory));

  spdlog::info("PAK Finalized. Total Files: {}", s_Entries.size());
  s_Entries.clear();
  return true;
}

} // namespace Horse

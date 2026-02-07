#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
#include <fstream>
#include <iostream>
#include <physfs.h>

namespace Horse {

static bool s_Initialized = false;

static void CanonicalizePhysFSPath(std::string &path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  while (path.size() >= 2 && path[0] == '.' && path[1] == '/') {
    path = path.substr(2);
  }
}

bool FileSystem::Initialize(const char *argv0) {
  if (s_Initialized)
    return true;

  if (PHYSFS_init(argv0) == 0) {
    HORSE_LOG_CORE_ERROR("PhysFS Init failed: {}", PHYSFS_getLastError());
    return false;
  }

  HORSE_LOG_CORE_INFO("FileSystem Initialized.");
  s_Initialized = true;
  return true;
}

void FileSystem::Shutdown() {
  if (s_Initialized) {
    PHYSFS_deinit();
    s_Initialized = false;
  }
}

bool FileSystem::Mount(const std::string &archive,
                       const std::string &mountPoint) {
  if (PHYSFS_mount(archive.c_str(), mountPoint.c_str(), 1) == 0) {
    HORSE_LOG_CORE_ERROR("Failed to mount {}: {}", archive,
                         PHYSFS_getLastError());
    return false;
  }
  HORSE_LOG_CORE_INFO("Mounted: {}", archive);
  return true;
}

bool FileSystem::ReadBytes(const std::filesystem::path &path,
                           std::vector<uint8_t> &outData) {
  std::string pathStr = path.string();
  CanonicalizePhysFSPath(pathStr);

  if (s_Initialized) {
    if (PHYSFS_exists(pathStr.c_str())) {
      PHYSFS_File *file = PHYSFS_openRead(pathStr.c_str());
      if (file) {
        PHYSFS_sint64 len = PHYSFS_fileLength(file);
        if (len >= 0) {
          outData.resize(len);
          PHYSFS_readBytes(file, outData.data(), len);
          PHYSFS_close(file);
          return true;
        }
        PHYSFS_close(file);
      }
    }
  }

  // Fallback directly to std::filesystem (for tools or absolute paths if not in
  // PhysFS) Note: PhysFS only sees mounted dirs. If Editor uses absolute paths,
  // we must fallback.
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream.is_open())
    return false;

  std::streamsize size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  outData.resize(size);
  stream.read(reinterpret_cast<char *>(outData.data()), size);
  return true;
}

bool FileSystem::ReadText(const std::filesystem::path &path,
                          std::string &outText) {
  std::vector<uint8_t> data;
  if (ReadBytes(path, data)) {
    outText.assign(reinterpret_cast<const char *>(data.data()), data.size());
    return true;
  }
  return false;
}

bool FileSystem::Exists(const std::filesystem::path &path) {
  std::string pathStr = path.string();
  CanonicalizePhysFSPath(pathStr);

  if (s_Initialized && PHYSFS_exists(pathStr.c_str())) {
    return true;
  }
  return std::filesystem::exists(path);
}

std::vector<std::string>
FileSystem::Enumerate(const std::filesystem::path &directory) {
  std::vector<std::string> results;
  std::string pathStr = directory.string();
  CanonicalizePhysFSPath(pathStr);

  if (s_Initialized) {
    char **rc = PHYSFS_enumerateFiles(pathStr.c_str());
    char **i;
    for (i = rc; *i != NULL; i++) {
      results.push_back(*i);
    }
    PHYSFS_freeList(rc);
  } else {
    // Fallback
    if (std::filesystem::exists(directory) &&
        std::filesystem::is_directory(directory)) {
      for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        results.push_back(entry.path().filename().string());
      }
    }
  }
  return results;
}

} // namespace Horse

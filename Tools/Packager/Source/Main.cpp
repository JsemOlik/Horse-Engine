#include "PakWriter.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#pragma pack(push, 1)
struct ICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  DWORD dwImageOffset;
};

struct ICONDIR {
  WORD idReserved;
  WORD idType;
  WORD idCount;
  // ICONDIRENTRY idEntries[1];
};

struct GRPICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  WORD nID;
};

struct GRPICONDIR {
  WORD idReserved;
  WORD idType;
  WORD idCount;
  // GRPICONDIRENTRY idEntries[1];
};
#pragma pack(pop)
#endif

#ifdef _WIN32
bool InjectIcon(const std::filesystem::path &exePath,
                const std::filesystem::path &iconPath) {
  if (!std::filesystem::exists(iconPath))
    return false;

  std::ifstream file(iconPath, std::ios::binary);
  if (!file)
    return false;

  ICONDIR dir;
  file.read((char *)&dir, sizeof(ICONDIR));
  if (dir.idType != 1)
    return false;

  std::vector<ICONDIRENTRY> entries(dir.idCount);
  file.read((char *)entries.data(), dir.idCount * sizeof(ICONDIRENTRY));

  HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
  if (!hUpdate)
    return false;

  std::vector<BYTE> grpBuffer(sizeof(GRPICONDIR) +
                              dir.idCount * sizeof(GRPICONDIRENTRY));
  GRPICONDIR *grpDir = (GRPICONDIR *)grpBuffer.data();
  grpDir->idReserved = dir.idReserved;
  grpDir->idType = dir.idType;
  grpDir->idCount = dir.idCount;

  GRPICONDIRENTRY *grpEntries =
      (GRPICONDIRENTRY *)(grpBuffer.data() + sizeof(GRPICONDIR));

  for (int i = 0; i < dir.idCount; ++i) {
    std::vector<BYTE> imgBuffer(entries[i].dwBytesInRes);
    file.seekg(entries[i].dwImageOffset);
    file.read((char *)imgBuffer.data(), entries[i].dwBytesInRes);

    if (!UpdateResourceW(hUpdate, (LPCWSTR)RT_ICON, MAKEINTRESOURCEW(i + 1),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         imgBuffer.data(), entries[i].dwBytesInRes)) {
      EndUpdateResourceW(hUpdate, TRUE);
      return false;
    }

    grpEntries[i].bWidth = entries[i].bWidth;
    grpEntries[i].bHeight = entries[i].bHeight;
    grpEntries[i].bColorCount = entries[i].bColorCount;
    grpEntries[i].bReserved = entries[i].bReserved;
    grpEntries[i].wPlanes = entries[i].wPlanes;
    grpEntries[i].wBitCount = entries[i].wBitCount;
    grpEntries[i].dwBytesInRes = entries[i].dwBytesInRes;
    grpEntries[i].nID = (WORD)(i + 1);
  }

  if (!UpdateResourceW(hUpdate, (LPCWSTR)RT_GROUP_ICON, L"MAINICON",
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                       grpBuffer.data(), (DWORD)grpBuffer.size())) {
    EndUpdateResourceW(hUpdate, TRUE);
    return false;
  }

  return EndUpdateResourceW(hUpdate, FALSE);
}
#endif

void PrintUsage() {
  std::cout << "Usage: HorsePackager <CookedAssetsDir> <OutputDir> "
               "<RuntimeExe> <GameDLL> [GameName] [IconPath]"
            << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 5) {
    PrintUsage();
    return 1;
  }

  std::filesystem::path cookedDir = std::filesystem::absolute(argv[1]);
  std::filesystem::path outputDir = std::filesystem::absolute(argv[2]);
  std::filesystem::path runtimeExe = std::filesystem::absolute(argv[3]);
  std::filesystem::path gameDll = std::filesystem::absolute(argv[4]);
  std::string gameName = "MyGame";
  if (argc >= 6) {
    gameName = argv[5];
  }

  std::filesystem::path iconPath;
  if (argc >= 7) {
    iconPath = std::filesystem::absolute(argv[6]);
  }

  if (!std::filesystem::exists(cookedDir)) {
    std::cerr << "Error: Cooked directory does not exist: " << cookedDir
              << std::endl;
    return 1;
  }

  // Create output directory
  if (!std::filesystem::exists(outputDir)) {
    std::filesystem::create_directories(outputDir);
  }

  // 1. Create Game.pak
  std::filesystem::path pakPath = outputDir / "Game.pak";
  Horse::PakWriter writer(pakPath);

  std::cout << "Packing assets from: " << cookedDir << std::endl;

  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(cookedDir)) {
    if (entry.is_regular_file()) {
      std::filesystem::path relativePath =
          std::filesystem::relative(entry.path(), cookedDir);
      std::string pathStr = relativePath.string();
      // Normalize separators to /
      std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

      writer.AddFile(entry.path(), pathStr);
    }
  }

  if (!writer.Finalize()) {
    std::cerr << "Error: Failed to finalize PAK file." << std::endl;
    return 1;
  }

  // 2. Copy Runtime Exe -> [GameName].exe
  try {
    std::string exeName = gameName + ".exe";
    std::filesystem::copy_file(
        runtimeExe, outputDir / exeName,
        std::filesystem::copy_options::overwrite_existing);
    std::cout << "Copied Runtime to " << exeName << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Error copying runtime: " << e.what() << std::endl;
  }

  // 3. Copy Game DLL
  try {
    std::filesystem::path exePath = outputDir / (gameName + ".exe");
    if (!iconPath.empty()) {
      std::cout << "Injecting icon: " << iconPath << std::endl;
#ifdef _WIN32
      if (!InjectIcon(exePath, iconPath)) {
        std::cerr << "Warning: Failed to inject icon." << std::endl;
      }
#else
      std::cerr << "Warning: Icon injection only supported on Windows."
                << std::endl;
#endif
    }

    std::filesystem::copy_file(
        gameDll, outputDir / "HorseGame.dll",
        std::filesystem::copy_options::overwrite_existing);
    std::cout << "Copied Game DLL" << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Error copying Game DLL: " << e.what() << std::endl;
  }

  // 4. Copy Dependencies
  std::filesystem::path runtimeDir = runtimeExe.parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(runtimeDir)) {
    if (entry.path().extension() == ".dll" &&
        entry.path().filename() != "HorseGame.dll") {
      try {
        std::filesystem::copy_file(
            entry.path(), outputDir / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing);
      } catch (...) {
      }
    }
  }

  // 5. Copy Engine assets
  std::filesystem::path buildEngineDir = runtimeDir / "Engine";
  if (std::filesystem::exists(buildEngineDir)) {
    try {
      std::filesystem::copy(
          buildEngineDir, outputDir / "Engine",
          std::filesystem::copy_options::recursive |
              std::filesystem::copy_options::overwrite_existing);
      std::cout << "Copied Engine assets" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error copying Engine assets: " << e.what() << std::endl;
    }
  }

  std::cout << "Packaging Complete!" << std::endl;
  return 0;
}

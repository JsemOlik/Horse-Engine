#include "PakWriter.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

void PrintUsage() {
  std::cout << "Usage: HorsePackager <CookedAssetsDir> <OutputDir> "
               "<RuntimeExe> <GameDLL>"
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

  // 2. Copy Runtime Exe -> MyGame.exe
  try {
    std::filesystem::copy_file(
        runtimeExe, outputDir / "MyGame.exe",
        std::filesystem::copy_options::overwrite_existing);
    std::cout << "Copied Runtime to MyGame.exe" << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Error copying runtime: " << e.what() << std::endl;
  }

  // 3. Copy Game DLL
  try {
    std::filesystem::copy_file(
        gameDll, outputDir / "HorseGame.dll",
        std::filesystem::copy_options::overwrite_existing);
    std::cout << "Copied Game DLL" << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Error copying Game DLL: " << e.what() << std::endl;
  }

  // 4. Copy Dependencies (Quick hack: copy all DLLs from Runtime dir)
  // In a real scenario, we'd know exactly what to copy.
  // For now, let's copy *.dll from runtime dir to output dir, skipping
  // HorseGame.dll as we handled it (or overwriting is fine).
  std::filesystem::path runtimeDir = runtimeExe.parent_path();
  for (const auto &entry : std::filesystem::directory_iterator(runtimeDir)) {
    if (entry.path().extension() == ".dll" &&
        entry.path().filename() !=
            "HorseGame.dll") { // Avoid copying old game dll if present
      try {
            entry.path(), outputDir / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing);
      } catch (...) {
      }
    }
  }

  std::cout << "Packaging Complete!" << std::endl;
  return 0;
}

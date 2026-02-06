#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <windows.h>


int main(int argc, char **argv) {
  // Initialize logging immediately
  Horse::Logger::Initialize();

  if (argc > 0) {
    std::cout << "Starting HorseRunner from: " << argv[0] << std::endl;

    // Initialize PhysFS
    if (!Horse::FileSystem::Initialize(argv[0])) {
      std::cerr << "Failed to initialize FileSystem via PhysFS" << std::endl;
      // Continue? Engine might use fallback.
    }

    // Determine executable directory
    std::filesystem::path exePath(argv[0]);
    std::string exeDir = exePath.parent_path().string();

    // Mount Game.pak
    // ...
    Horse::FileSystem::Mount(exeDir, "/");

    // Initialize AssetManager (Runtime assumes Root /)
    Horse::AssetManager::Get().Initialize(".");

    // Mount Game.pak
    // It should be adjacent to executable
    std::filesystem::path pakPath = exePath.parent_path() / "Game.pak";
    std::string pakPathStr = pakPath.string();

    if (Horse::FileSystem::Exists(pakPath)) {
      std::cout << "Found Game.pak at " << pakPathStr << ". Mounting..."
                << std::endl;
      if (Horse::FileSystem::Mount(pakPathStr, "/")) {
        std::cout << "Successfully mounted Game.pak" << std::endl;
      } else {
        std::cerr << "Failed to mount Game.pak" << std::endl;
      }
    } else {
      std::cout << "Game.pak not found via FileSystem::Exists. Checking "
                   "std::filesystem..."
                << std::endl;
      if (std::filesystem::exists(pakPath)) {
        // Try mounting via absolute path logic if Exists fails (e.g. init
        // issue)
        Horse::FileSystem::Mount(pakPathStr, "/");
      }
    }
  }

  // Check for headless mode
  bool headless = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--headless") == 0) {
      headless = true;
    }
  }

  try {
    auto engine = new Horse::Engine();
    if (engine->Initialize(headless)) {
      std::cout << "Engine Initialized. Starting Run Loop..." << std::endl;
      engine->Run();
      std::cout << "Engine Run Loop execution finished." << std::endl;
    } else {
      std::cerr << "Engine Initialize failed!" << std::endl;
    }

    Horse::FileSystem::Shutdown();
    delete engine;
  } catch (const std::exception &e) {
    std::cerr << "Unhandled exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown unhandled exception!" << std::endl;
  }

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  return 0;
}

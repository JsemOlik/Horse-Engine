#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Project/Project.h"
#include "HorseEngine/Render/D3D11Renderer.h"
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
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
        Horse::FileSystem::Mount(pakPathStr, "/");
      }
    }

    // Load Project Config
    std::vector<uint8_t> projectData;
    if (Horse::FileSystem::ReadBytes("Game.project.bin", projectData)) {
#pragma pack(push, 1)
      struct ProjectCookedHeader {
        char Magic[4];
        uint32_t Version;
        uint64_t DefaultLevelGUID;
      };
#pragma pack(pop)

      if (projectData.size() >= sizeof(ProjectCookedHeader)) {
        auto header =
            reinterpret_cast<ProjectCookedHeader *>(projectData.data());
        if (memcmp(header->Magic, "HPRJ", 4) == 0) {
          auto project = std::make_shared<Horse::Project>();
          auto &config = project->GetConfig();

          HORSE_LOG_CORE_INFO("Project Bin Loaded. DefaultLevelGUID: {0}",
                              header->DefaultLevelGUID);

          // Load Manifest to resolve GUID
          std::string manifestContent;
          if (Horse::FileSystem::ReadText("Game.manifest.json",
                                          manifestContent)) {
            try {
              auto manifest = nlohmann::json::parse(manifestContent);
              std::string guidStr = std::to_string(header->DefaultLevelGUID);
              if (manifest.contains("Assets")) {
                auto &assets = manifest["Assets"];
                if (assets.contains(guidStr)) {
                  config.DefaultScene = assets[guidStr].get<std::string>();
                  HORSE_LOG_CORE_INFO("Resolved Default Scene: {0}",
                                      config.DefaultScene);
                } else {
                  HORSE_LOG_CORE_WARN(
                      "GUID {0} not found in Game.manifest.json", guidStr);

                  // Diagnostic: Log some available manifest keys
                  HORSE_LOG_CORE_INFO(
                      "Checking first 10 manifest entries for comparison...");
                  int count = 0;
                  for (auto &[key, val] : assets.items()) {
                    HORSE_LOG_CORE_INFO("  Manifest Key: {0} -> {1}", key,
                                        val.get<std::string>());
                    if (++count >= 10)
                      break;
                  }
                }
              }
            } catch (...) {
              HORSE_LOG_CORE_ERROR("Failed to parse Game.manifest.json");
            }
          }

          Horse::Project::SetActive(project);
        }
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

      std::unique_ptr<Horse::D3D11Renderer> renderer;
      if (!headless) {
        renderer = std::make_unique<Horse::D3D11Renderer>();
        Horse::RendererDesc desc;
        desc.WindowHandle = engine->GetWindow()->GetNativeWindow();
        desc.Width = engine->GetWindow()->GetWidth();
        desc.Height = engine->GetWindow()->GetHeight();
        desc.VSync = true;

        if (renderer->Initialize(desc)) {
          engine->SetRenderer(renderer.get());
        } else {
          std::cerr << "Failed to initialize Renderer!" << std::endl;
        }
      }

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

#include "HorseEngine/Project/ProjectSerializer.h"
#include "HorseEngine/Project/Project.h"

#include "HorseEngine/Core/FileSystem.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>


using json = nlohmann::json;

namespace Horse {

ProjectSerializer::ProjectSerializer(std::shared_ptr<Project> project)
    : m_Project(project) {}

bool ProjectSerializer::SerializeToJSON(const std::string &filepath) {
  const auto &config = m_Project->GetConfig();

  json projectJson;
  projectJson["name"] = config.Name;
  projectJson["guid"] = config.GUID;
  projectJson["engineVersion"] = config.EngineVersion;
  projectJson["defaultScene"] = config.DefaultScene;
  projectJson["assetDirectory"] = config.AssetDirectory;

  std::ofstream fout(filepath);
  if (!fout.is_open())
    return false;

  fout << projectJson.dump(4);
  return true;
}

bool ProjectSerializer::DeserializeFromJSON(const std::string &filepath) {
  std::ifstream f(filepath);
  if (!f.is_open())
    return false;

  json projectJson;
  try {
    projectJson = json::parse(f);
  } catch (const json::parse_error &e) {
    std::cerr << "Project parse error: " << e.what() << std::endl;
    return false;
  }

  auto &config = m_Project->GetConfig();
  config.Name = projectJson.value("name", "Untitled Project");
  config.GUID = projectJson.value("guid", "");
  config.EngineVersion = projectJson.value("engineVersion", "0.1.0");
  config.DefaultScene = projectJson.value("defaultScene", "");
  config.AssetDirectory = projectJson.value("assetDirectory", "Assets");

  std::filesystem::path path(filepath);
  config.ProjectFileName = path.filename();
  config.ProjectDirectory = path.parent_path();

  return true;
}

struct ProjectCookedHeader {
  char Magic[4];
  uint32_t Version;
  uint64_t DefaultLevelGUID;
};

std::shared_ptr<Project>
Project::LoadFromBinary(const std::filesystem::path &path) {
  std::vector<uint8_t> data;
  if (!FileSystem::ReadBytes(path, data))
    return nullptr;

  if (data.size() < sizeof(ProjectCookedHeader))
    return nullptr;

  const ProjectCookedHeader *header =
      reinterpret_cast<const ProjectCookedHeader *>(data.data());

  if (header->Magic[0] != 'H' || header->Magic[1] != 'P' ||
      header->Magic[2] != 'R' || header->Magic[3] != 'J') {
    return nullptr;
  }

  auto project = std::make_shared<Project>();
  auto &config = project->GetConfig();
  config.DefaultLevelGUID = header->DefaultLevelGUID;
  config.IsCooked = true; // Mark as cooked/packaged

  // Set internal paths (useful for resolving relative to project)
  // In standalone, we assume project directory is the same as the EXE/PAK
  // directory
  config.ProjectFileName = path.filename();
  config.ProjectDirectory = std::filesystem::current_path();

  return project;
}

} // namespace Horse

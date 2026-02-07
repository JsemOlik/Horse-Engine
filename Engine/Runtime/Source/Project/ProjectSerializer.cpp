#include "HorseEngine/Project/ProjectSerializer.h"
#include "HorseEngine/Project/Project.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

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

} // namespace Horse

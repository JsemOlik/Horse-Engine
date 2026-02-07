#include "HorseEngine/Project/ProjectSerializer.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
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
  std::string jsonContent;
  if (!FileSystem::ReadText(filepath, jsonContent))
    return false;

  json projectJson;
  try {
    projectJson = json::parse(jsonContent);
  } catch (const json::parse_error &e) {
    HORSE_LOG_CORE_ERROR("Project parse error: {}", e.what());
    return false;
  }

  auto &config = m_Project->GetConfig();
  if (projectJson.is_object()) {
    config.Name = projectJson.value("name", "Untitled Project");
    config.GUID = projectJson.value("guid", "");
    config.EngineVersion = projectJson.value("engineVersion", "0.1.0");
    config.DefaultScene = projectJson.value("defaultScene", "");
    config.AssetDirectory = projectJson.value("assetDirectory", "Assets");
  }

  std::filesystem::path path(filepath);
  config.ProjectFileName = path.filename();
  config.ProjectDirectory = path.parent_path();

  return true;
}

struct ProjectCookedHeader {
  char Magic[4];
  uint32_t Version;
  char Name[128];
  uint64_t DefaultLevelGUID;
};

bool ProjectSerializer::DeserializeFromBinary(const std::string &filepath) {
  std::vector<uint8_t> data;
  if (!FileSystem::ReadBytes(filepath, data))
    return false;

  if (data.size() < sizeof(ProjectCookedHeader))
    return false;

  ProjectCookedHeader header;
  memcpy(&header, data.data(), sizeof(ProjectCookedHeader));

  if (header.Magic[0] != 'H' || header.Magic[1] != 'P' ||
      header.Magic[2] != 'R' || header.Magic[3] != 'J') {
    HORSE_LOG_CORE_ERROR("Invalid project binary: {}", filepath);
    return false;
  }

  auto &config = m_Project->GetConfig();
  config.Name = std::string(header.Name);
  config.DefaultLevelGUID = header.DefaultLevelGUID;

  std::filesystem::path path(filepath);
  config.ProjectFileName = path.filename();
  config.ProjectDirectory = path.parent_path();

  return true;
}

} // namespace Horse

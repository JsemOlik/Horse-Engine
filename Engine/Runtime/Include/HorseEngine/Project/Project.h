#pragma once

#include <filesystem>
#include <memory>
#include <string>


namespace Horse {

struct ProjectConfig {
  std::string Name = "Untitled Project";
  std::string GUID;
  std::string EngineVersion = "0.1.0";
  std::string DefaultScene;
  std::string AssetDirectory = "Assets";

  // Internal paths
  std::filesystem::path ProjectFileName;
  std::filesystem::path ProjectDirectory;
};

class Project {
public:
  Project() = default;
  ~Project() = default;

  const ProjectConfig &GetConfig() const { return m_Config; }
  ProjectConfig &GetConfig() { return m_Config; }

  static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }
  static void SetActive(std::shared_ptr<Project> project) {
    s_ActiveProject = project;
  }

  static std::filesystem::path GetProjectDirectory() {
    return s_ActiveProject ? s_ActiveProject->m_Config.ProjectDirectory : "";
  }

  static std::filesystem::path GetAssetDirectory() {
    if (!s_ActiveProject)
      return "";
    return s_ActiveProject->m_Config.ProjectDirectory /
           s_ActiveProject->m_Config.AssetDirectory;
  }

private:
  ProjectConfig m_Config;
  inline static std::shared_ptr<Project> s_ActiveProject = nullptr;
};

} // namespace Horse

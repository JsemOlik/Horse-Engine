#pragma once

#include "HorseEngine/Core.h"
#include <memory>
#include <string>

namespace Horse {

class Project;

class HORSE_API ProjectSerializer {
public:
  ProjectSerializer(std::shared_ptr<Project> project);

  bool SerializeToJSON(const std::string &filepath);
  bool DeserializeFromJSON(const std::string &filepath);
  bool DeserializeFromBinary(const std::string &filepath);

private:
  std::shared_ptr<Project> m_Project;
};

} // namespace Horse

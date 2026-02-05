#pragma once

#include "HorseEngine/Core.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Horse {

class HORSE_API Prefab {
public:
  Prefab() = default;
  ~Prefab() = default;

  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }

  const nlohmann::json &GetTemplateData() const { return m_TemplateData; }
  void SetTemplateData(const nlohmann::json &data) { m_TemplateData = data; }

private:
  std::string m_Name;
  nlohmann::json m_TemplateData; // Full entity hierarchy as JSON
};

} // namespace Horse

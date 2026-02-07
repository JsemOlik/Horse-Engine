#include "HorseEngine/Render/MaterialSerializer.h"
#include "HorseEngine/Core/Logging.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace Horse {

bool MaterialSerializer::Serialize(const MaterialInstance &material,
                                   const std::string &filepath) {
  nlohmann::json out;
  out["Name"] = material.GetName();
  out["Shader"] = material.GetShaderName();

  nlohmann::json floatProps = nlohmann::json::object();
  for (const auto &[name, value] : material.GetFloatProperties()) {
    floatProps[name] = value;
  }
  out["FloatProperties"] = floatProps;

  nlohmann::json colorProps = nlohmann::json::object();
  for (const auto &[name, value] : material.GetColorProperties()) {
    colorProps[name] = value;
  }
  out["ColorProperties"] = colorProps;

  nlohmann::json texProps = nlohmann::json::object();
  for (const auto &[name, value] : material.GetTextureProperties()) {
    texProps[name] = value;
  }
  out["TextureProperties"] = texProps;

  std::ofstream fout(filepath);
  if (!fout.is_open()) {
    HORSE_LOG_CORE_ERROR("Could not open file for writing: {0}", filepath);
    return false;
  }

  fout << out.dump(4);
  return true;
}

bool MaterialSerializer::Deserialize(const std::string &filepath,
                                     MaterialInstance &output) {
  std::ifstream stream(filepath);
  if (!stream.is_open()) {
    HORSE_LOG_CORE_ERROR("Could not open file for reading: {0}", filepath);
    return false;
  }

  nlohmann::json data;
  try {
    stream >> data;
  } catch (const nlohmann::json::exception &e) {
    HORSE_LOG_CORE_ERROR("Failed to parse material JSON: {0}", e.what());
    return false;
  }

  if (data.contains("Name"))
    output.SetName(data["Name"]);

  if (data.contains("Shader"))
    output.SetShaderName(data["Shader"]);

  if (data.contains("FloatProperties") && data["FloatProperties"].is_object()) {
    for (auto &[key, value] : data["FloatProperties"].items()) {
      output.SetFloat(key, value);
    }
  }

  if (data.contains("ColorProperties") && data["ColorProperties"].is_object()) {
    for (auto &[key, value] : data["ColorProperties"].items()) {
      if (value.is_array() && value.size() == 4) {
        std::array<float, 4> color = {value[0], value[1], value[2], value[3]};
        output.SetColor(key, color);
      }
    }
  }

  if (data.contains("TextureProperties") &&
      data["TextureProperties"].is_object()) {
    for (auto &[key, value] : data["TextureProperties"].items()) {
      output.SetTexture(key, value);
    }
  }

  return true;
}

} // namespace Horse

#include "HorseEngine/Render/MaterialSerializer.h"
#include "HorseEngine/Core/FileSystem.h"
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
  std::vector<uint8_t> data;
  if (!FileSystem::ReadBytes(filepath, data)) {
    HORSE_LOG_CORE_ERROR("Could not open file for reading: {0}", filepath);
    return false;
  }

  if (data.empty())
    return false;

  std::string jsonContent;

  // Check for Cooked Header
  if (data.size() >= sizeof(MaterialCookedHeader)) {
    const MaterialCookedHeader *header =
        reinterpret_cast<const MaterialCookedHeader *>(data.data());
    if (memcmp(header->Magic, "HMAT", 4) == 0) {
      HORSE_LOG_CORE_INFO("MaterialSerializer: Detected cooked header for {0}",
                          filepath);
      // Cooked material: skip header and size prefix
      size_t offset = sizeof(MaterialCookedHeader);
      if (data.size() >= offset + sizeof(uint32_t)) {
        uint32_t jsonSize =
            *reinterpret_cast<const uint32_t *>(data.data() + offset);
        offset += sizeof(uint32_t);

        if (data.size() >= offset + jsonSize) {
          jsonContent.assign(
              reinterpret_cast<const char *>(data.data() + offset), jsonSize);
          HORSE_LOG_CORE_INFO(
              "MaterialSerializer: Extracted JSON segment ({0} bytes)",
              jsonSize);
        } else {
          HORSE_LOG_CORE_ERROR(
              "MaterialSerializer: JSON segment size mismatch for {0}",
              filepath);
        }
      }
    }
  }

  // Fallback if not cooked or header check failed
  if (jsonContent.empty()) {
    HORSE_LOG_CORE_INFO("MaterialSerializer: Treating {0} as raw JSON",
                        filepath);
    jsonContent.assign(reinterpret_cast<const char *>(data.data()),
                       data.size());
  }

  nlohmann::json j;
  try {
    j = nlohmann::json::parse(jsonContent);
  } catch (const nlohmann::json::exception &e) {
    HORSE_LOG_CORE_ERROR("Failed to parse material JSON from {0}: {1}",
                         filepath, e.what());
    return false;
  }

  if (j.contains("Name"))
    output.SetName(j["Name"]);

  if (j.contains("Shader"))
    output.SetShaderName(j["Shader"]);

  if (j.contains("FloatProperties")) {
    for (auto &[key, value] : j["FloatProperties"].items()) {
      output.SetFloat(key, value);
    }
  }

  if (j.contains("ColorProperties")) {
    for (auto &[key, value] : j["ColorProperties"].items()) {
      if (value.is_array() && value.size() == 4) {
        std::array<float, 4> color = {value[0], value[1], value[2], value[3]};
        output.SetColor(key, color);
      }
    }
  }

  if (j.contains("TextureProperties")) {
    for (auto &[key, value] : j["TextureProperties"].items()) {
      output.SetTexture(key, value);
    }
  }

  return true;
}

} // namespace Horse

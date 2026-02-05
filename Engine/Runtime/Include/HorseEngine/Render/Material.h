#pragma once

#include "HorseEngine/Core.h"
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace Horse {

class HORSE_API MaterialInstance {
public:
  MaterialInstance(const std::string &name = "Untitled Material");
  ~MaterialInstance() = default;

  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }

  const std::string &GetShaderName() const { return m_ShaderName; }
  void SetShaderName(const std::string &shaderName) {
    m_ShaderName = shaderName;
  }

  // Property Getters/Setters
  void SetFloat(const std::string &name, float value);
  float GetFloat(const std::string &name) const;

  void SetColor(const std::string &name, const std::array<float, 4> &value);
  std::array<float, 4> GetColor(const std::string &name) const;

  void SetTexture(const std::string &name, const std::string &pathOrAssetID);
  std::string GetTexture(const std::string &name) const;
  bool HasTexture(const std::string &name) const;

  const std::string &GetFilePath() const { return m_FilePath; }
  void SetFilePath(const std::string &path) { m_FilePath = path; }

  // Access to raw maps for serialization/iteration
  const std::unordered_map<std::string, float> &GetFloatProperties() const {
    return m_FloatProps;
  }
  const std::unordered_map<std::string, std::array<float, 4>> &
  GetColorProperties() const {
    return m_ColorProps;
  }
  const std::unordered_map<std::string, std::string> &
  GetTextureProperties() const {
    return m_TextureProps;
  }

private:
  std::string m_Name;
  std::string m_FilePath;
  std::string m_ShaderName;

  std::unordered_map<std::string, float> m_FloatProps;
  std::unordered_map<std::string, std::array<float, 4>> m_ColorProps;
  std::unordered_map<std::string, std::string> m_TextureProps;
};

} // namespace Horse

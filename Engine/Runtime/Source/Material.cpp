#include "HorseEngine/Render/Material.h"

namespace Horse {

Material::Material(const std::string &name)
    : m_Name(name), m_ShaderName("StandardPBR") {}

void Material::SetFloat(const std::string &name, float value) {
  m_FloatProps[name] = value;
}

float Material::GetFloat(const std::string &name) const {
  auto it = m_FloatProps.find(name);
  if (it != m_FloatProps.end()) {
    return it->second;
  }
  return 0.0f;
}

void Material::SetColor(const std::string &name,
                        const std::array<float, 4> &value) {
  m_ColorProps[name] = value;
}

std::array<float, 4> Material::GetColor(const std::string &name) const {
  auto it = m_ColorProps.find(name);
  if (it != m_ColorProps.end()) {
    return it->second;
  }
  return {1.0f, 1.0f, 1.0f, 1.0f};
}

void Material::SetTexture(const std::string &name,
                          const std::string &pathOrAssetID) {
  m_TextureProps[name] = pathOrAssetID;
}

std::string Material::GetTexture(const std::string &name) const {
  auto it = m_TextureProps.find(name);
  if (it != m_TextureProps.end()) {
    return it->second;
  }
  return "";
}

} // namespace Horse

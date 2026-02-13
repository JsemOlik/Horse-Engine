#include "HorseEngine/Render/D3D11Texture.h"
#include "HorseEngine/Core/FileSystem.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Project/Project.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Horse {

bool D3D11Texture::LoadFromFile(ID3D11Device *device,
                                ID3D11DeviceContext *context,
                                const std::string &filePath, bool srgb,
                                bool generateMips) {
  namespace fs = std::filesystem;
  
  // Try loading through FileSystem (supports PAK files)
  std::vector<uint8_t> fileData;
  bool loaded = FileSystem::ReadBytes(filePath, fileData);
  
  // If FileSystem fails, try legacy path resolution for editor
  if (!loaded) {
    fs::path texturePath = filePath;
    
    // In editor mode, try project directory first
    if (texturePath.is_relative() && !Project::IsCooked()) {
      auto projectDir = Project::GetProjectDirectory();
      if (!projectDir.empty()) {
        fs::path projectTexturePath = projectDir / texturePath;
        if (fs::exists(projectTexturePath)) {
          texturePath = projectTexturePath;
        }
      }
    }
    
    // Try direct path
    if (!fs::exists(texturePath)) {
      // Try relative to executable
      wchar_t exePath[MAX_PATH];
      GetModuleFileNameW(NULL, exePath, MAX_PATH);
      fs::path exeDir = fs::path(exePath).parent_path();
      fs::path relativePath = exeDir / texturePath;

      if (fs::exists(relativePath)) {
        texturePath = relativePath;
      } else {
        // Try climbing up from exe
        fs::path searchDir = exeDir;
        bool found = false;
        for (int i = 0; i < 4; ++i) {
          if (fs::exists(searchDir / texturePath)) {
            texturePath = searchDir / texturePath;
            found = true;
            break;
          }
          if (searchDir.has_parent_path())
            searchDir = searchDir.parent_path();
          else
            break;
        }

        if (!found) {
          HORSE_LOG_RENDER_ERROR("Failed to find texture file: {}", filePath);
          return false;
        }
      }
    }
    
    // Read file using standard filesystem
    std::ifstream file(texturePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      HORSE_LOG_RENDER_ERROR("Failed to open texture file: {}", texturePath.string());
      return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    fileData.resize(size);
    file.read(reinterpret_cast<char*>(fileData.data()), size);
  }

  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), 
                           &width, &height, &channels, 4);

  if (!data) {
    HORSE_LOG_RENDER_ERROR("Failed to decode texture: {}", filePath);
    return false;
  }

  m_Width = static_cast<u32>(width);
  m_Height = static_cast<u32>(height);

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = m_Width;
  textureDesc.Height = m_Height;
  textureDesc.MipLevels = generateMips ? 0 : 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format =
      srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.SampleDesc.Quality = 0;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  if (generateMips) {
    textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
  }

  ComPtr<ID3D11Texture2D> texture;
  HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &texture);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create D3D11 texture from: {}", filePath);
    stbi_image_free(data);
    return false;
  }

  context->UpdateSubresource(texture.Get(), 0, nullptr, data, m_Width * 4, 0);
  stbi_image_free(data);

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = textureDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.Texture2D.MipLevels = -1;

  hr = device->CreateShaderResourceView(texture.Get(), &srvDesc,
                                        &m_ShaderResourceView);
  if (FAILED(hr))
    return false;

  if (generateMips) {
    context->GenerateMips(m_ShaderResourceView.Get());
  }

  // Create Sampler
  D3D11_SAMPLER_DESC samplerDesc = {};
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

  hr = device->CreateSamplerState(&samplerDesc, &m_SamplerState);
  if (FAILED(hr))
    return false;

  return true;
}

bool D3D11Texture::Create(ID3D11Device *device, u32 width, u32 height,
                          const void *data, DXGI_FORMAT format) {
  m_Width = width;
  m_Height = height;

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = m_Width;
  textureDesc.Height = m_Height;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = format;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA subData = {};
  subData.pSysMem = data;
  subData.SysMemPitch = width * 4; // Assuming 4 bytes per pixel for now

  ComPtr<ID3D11Texture2D> texture;
  HRESULT hr = device->CreateTexture2D(&textureDesc, data ? &subData : nullptr,
                                       &texture);
  if (FAILED(hr))
    return false;

  hr = device->CreateShaderResourceView(texture.Get(), nullptr,
                                        &m_ShaderResourceView);
  if (FAILED(hr))
    return false;

  D3D11_SAMPLER_DESC samplerDesc = {};
  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

  hr = device->CreateSamplerState(&samplerDesc, &m_SamplerState);
  if (FAILED(hr))
    return false;

  return true;
}

bool D3D11Texture::LoadCubemap(ID3D11Device *device,
                               const std::vector<std::string> &facePaths) {
  if (facePaths.size() != 6)
    return false;

  m_IsCubemap = true;
  // ... Implementation for cubemap loading would go here ...
  // For now, let's keep it as a placeholder to satisfy the header
  return true;
}

void D3D11Texture::Bind(ID3D11DeviceContext *context, u32 slot) {
  context->PSSetShaderResources(slot, 1, m_ShaderResourceView.GetAddressOf());
  context->PSSetSamplers(slot, 1, m_SamplerState.GetAddressOf());
}

} // namespace Horse

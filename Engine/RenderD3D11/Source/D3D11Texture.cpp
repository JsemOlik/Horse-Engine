#include "HorseEngine/Render/D3D11Texture.h"
#include "HorseEngine/Core/Logging.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Horse {

bool D3D11Texture::LoadFromFile(ID3D11Device *device,
                                const std::string &filePath) {
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data =
      stbi_load(filePath.c_str(), &width, &height, &channels, 4);

  if (!data) {
    HORSE_LOG_RENDER_ERROR("Failed to load texture: {}", filePath);
    return false;
  }

  m_Width = static_cast<u32>(width);
  m_Height = static_cast<u32>(height);

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = m_Width;
  textureDesc.Height = m_Height;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.SampleDesc.Quality = 0;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA subresourceData = {};
  subresourceData.pSysMem = data;
  subresourceData.SysMemPitch = m_Width * 4;

  ComPtr<ID3D11Texture2D> texture;
  HRESULT hr =
      device->CreateTexture2D(&textureDesc, &subresourceData, &texture);
  stbi_image_free(data);

  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create D3D11 texture from: {}", filePath);
    return false;
  }

  hr = device->CreateShaderResourceView(texture.Get(), nullptr,
                                        &m_ShaderResourceView);
  if (FAILED(hr))
    return false;

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

void D3D11Texture::Bind(ID3D11DeviceContext *context, u32 slot) {
  context->PSSetShaderResources(slot, 1, m_ShaderResourceView.GetAddressOf());
  context->PSSetSamplers(slot, 1, m_SamplerState.GetAddressOf());
}

} // namespace Horse

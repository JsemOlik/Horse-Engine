#pragma once

#include "HorseEngine/Core.h"
#include <d3d11.h>
#include <string>
#include <vector>
#include <wrl/client.h>

namespace Horse {

using Microsoft::WRL::ComPtr;

class D3D11Texture {
public:
  D3D11Texture() = default;
  ~D3D11Texture() = default;

  bool LoadFromFile(ID3D11Device *device, ID3D11DeviceContext *context,
                    const std::string &filePath, bool srgb = false,
                    bool generateMips = true);
  bool Create(ID3D11Device *device, u32 width, u32 height, const void *data,
              DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
  bool LoadCubemap(ID3D11Device *device,
                   const std::vector<std::string> &facePaths);

  void Bind(ID3D11DeviceContext *context, u32 slot = 0);

  ID3D11ShaderResourceView *GetSRV() const {
    return m_ShaderResourceView.Get();
  }
  ID3D11SamplerState *GetSampler() const { return m_SamplerState.Get(); }

  u32 GetWidth() const { return m_Width; }
  u32 GetHeight() const { return m_Height; }

private:
  ComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;
  ComPtr<ID3D11SamplerState> m_SamplerState;
  u32 m_Width = 0;
  u32 m_Height = 0;
  bool m_IsCubemap = false;
};

} // namespace Horse

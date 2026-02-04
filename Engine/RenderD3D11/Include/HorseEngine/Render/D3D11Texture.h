#pragma once

#include "HorseEngine/Core.h"
#include <d3d11.h>
#include <string>
#include <wrl/client.h>


namespace Horse {

using Microsoft::WRL::ComPtr;

class D3D11Texture {
public:
  D3D11Texture() = default;
  ~D3D11Texture() = default;

  bool LoadFromFile(ID3D11Device *device, const std::string &filePath);
  void Bind(ID3D11DeviceContext *context, u32 slot = 0);

  ID3D11ShaderResourceView *GetSRV() const {
    return m_ShaderResourceView.Get();
  }
  ID3D11SamplerState *GetSampler() const { return m_SamplerState.Get(); }

private:
  ComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;
  ComPtr<ID3D11SamplerState> m_SamplerState;
  u32 m_Width = 0;
  u32 m_Height = 0;
};

} // namespace Horse

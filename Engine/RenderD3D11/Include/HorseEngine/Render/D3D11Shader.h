#pragma once

#include "HorseEngine/Core.h"
#include <d3d11.h>
#include <string>
#include <vector>
#include <wrl/client.h>


namespace Horse {

using Microsoft::WRL::ComPtr;

class D3D11Shader {
public:
  D3D11Shader() = default;
  ~D3D11Shader() = default;

  bool CompileFromFile(ID3D11Device *device, const std::wstring &filepath,
                       const std::string &entryPoint,
                       const std::string &profile,
                       const std::vector<D3D_SHADER_MACRO> &defines = {});

  bool CompileFromSource(ID3D11Device *device, const std::string &source,
                         const std::string &entryPoint,
                         const std::string &profile,
                         const std::vector<D3D_SHADER_MACRO> &defines = {});

  ID3D11VertexShader *GetVertexShader() const { return m_VertexShader.Get(); }
  ID3D11PixelShader *GetPixelShader() const { return m_PixelShader.Get(); }
  ID3DBlob *GetBytecode() const { return m_Bytecode.Get(); }

private:
  bool CompileShader(ID3D11Device *device, const std::string &source,
                     const std::string &entryPoint, const std::string &profile,
                     const std::vector<D3D_SHADER_MACRO> &defines);

  ComPtr<ID3D11VertexShader> m_VertexShader;
  ComPtr<ID3D11PixelShader> m_PixelShader;
  ComPtr<ID3DBlob> m_Bytecode;
};

} // namespace Horse

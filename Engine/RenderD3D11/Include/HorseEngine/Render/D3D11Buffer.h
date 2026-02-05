#pragma once

#include "HorseEngine/Core.h"
#include <d3d11.h>
#include <wrl/client.h>

namespace Horse {

using Microsoft::WRL::ComPtr;

enum class BufferType { Vertex, Index, Constant, Structured };

enum class BufferUsage { Immutable, Dynamic, Default };

class D3D11Buffer {
public:
  D3D11Buffer() = default;
  ~D3D11Buffer() = default;

  bool Initialize(ID3D11Device *device, BufferType type, BufferUsage usage,
                  const void *data, size_t size, size_t stride = 0);

  void UpdateData(ID3D11DeviceContext *context, const void *data, size_t size);

  void Bind(ID3D11DeviceContext *context, u32 slot = 0);

  ID3D11Buffer *Get() const { return m_Buffer.Get(); }
  size_t GetSize() const { return m_Size; }
  size_t GetStride() const { return m_Stride; }

private:
  ComPtr<ID3D11Buffer> m_Buffer;
  BufferType m_Type = BufferType::Vertex;
  BufferUsage m_Usage = BufferUsage::Immutable;
  size_t m_Size = 0;
  size_t m_Stride = 0;
};

} // namespace Horse

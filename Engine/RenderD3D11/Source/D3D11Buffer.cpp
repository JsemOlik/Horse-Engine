#include "HorseEngine/Render/D3D11Buffer.h"
#include "HorseEngine/Core/Logging.h"

namespace Horse {

bool D3D11Buffer::Initialize(ID3D11Device *device, BufferType type,
                             BufferUsage usage, const void *data, size_t size,
                             size_t stride) {
  m_Type = type;
  m_Usage = usage;
  m_Size = size;
  m_Stride = stride;

  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = static_cast<UINT>(size);
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  switch (type) {
  case BufferType::Vertex:
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    break;
  case BufferType::Index:
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    break;
  case BufferType::Constant:
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    break;
  case BufferType::Structured:
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = static_cast<UINT>(stride);
    break;
  }

  switch (usage) {
  case BufferUsage::Immutable:
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.CPUAccessFlags = 0;
    break;
  case BufferUsage::Dynamic:
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    break;
  case BufferUsage::Default:
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    break;
  }

  D3D11_SUBRESOURCE_DATA subData = {};
  subData.pSysMem = data;

  HRESULT hr = device->CreateBuffer(&desc, data ? &subData : nullptr,
                                    m_Buffer.GetAddressOf());
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create D3D11 buffer!");
    return false;
  }

  return true;
}

void D3D11Buffer::UpdateData(ID3D11DeviceContext *context, const void *data,
                             size_t size) {
  if (m_Usage != BufferUsage::Dynamic) {
    HORSE_LOG_RENDER_ERROR("Attempting to update a non-dynamic buffer!");
    return;
  }

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr =
      context->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (SUCCEEDED(hr)) {
    memcpy(mapped.pData, data, size);
    context->Unmap(m_Buffer.Get(), 0);
  }
}

void D3D11Buffer::Bind(ID3D11DeviceContext *context, u32 slot) {
  UINT stride = static_cast<UINT>(m_Stride);
  UINT offset = 0;

  switch (m_Type) {
  case BufferType::Vertex:
    context->IASetVertexBuffers(slot, 1, m_Buffer.GetAddressOf(), &stride,
                                &offset);
    break;
  case BufferType::Index:
    context->IASetIndexBuffer(m_Buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    break;
  case BufferType::Constant:
    context->VSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
    context->PSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
    break;
  case BufferType::Structured:
    // Structured buffers are typically bound as SRVs
    break;
  }
}

} // namespace Horse

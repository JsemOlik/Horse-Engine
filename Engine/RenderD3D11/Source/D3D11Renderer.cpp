#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Render/D3D11Shader.h"
#include <DirectXMath.h>
#include <dxgi1_2.h>


using namespace DirectX;

namespace Horse {

D3D11Renderer::~D3D11Renderer() { Shutdown(); }

bool D3D11Renderer::Initialize(const RendererDesc &desc) {
  m_Width = desc.Width;
  m_Height = desc.Height;
  m_VSync = desc.VSync;

  if (!CreateDeviceAndSwapChain(desc)) {
    HORSE_LOG_RENDER_ERROR("Failed to create D3D11 device and swap chain");
    return false;
  }

  if (!CreateRenderTargetView()) {
    HORSE_LOG_RENDER_ERROR("Failed to create render target view");
    return false;
  }

  if (!CreateDepthStencilView()) {
    HORSE_LOG_RENDER_ERROR("Failed to create depth stencil view");
    return false;
  }

  m_Viewport.TopLeftX = 0.0f;
  m_Viewport.TopLeftY = 0.0f;
  m_Viewport.Width = static_cast<f32>(m_Width);
  m_Viewport.Height = static_cast<f32>(m_Height);
  m_Viewport.MinDepth = 0.0f;
  m_Viewport.MaxDepth = 1.0f;

  HORSE_LOG_RENDER_INFO("D3D11 Renderer initialized: {}x{}", m_Width, m_Height);
  return true;
}

void D3D11Renderer::Shutdown() {
  m_CubeVertexBuffer.Reset();
  m_CubeIndexBuffer.Reset();
  m_CubeConstantBuffer.Reset();
  m_CubeInputLayout.Reset();
  m_CubeVS.Reset();
  m_CubePS.Reset();

  m_DepthStencilView.Reset();
  m_DepthStencilBuffer.Reset();
  m_RenderTargetView.Reset();
  m_SwapChain.Reset();
  m_Context.Reset();
  m_Device.Reset();
}

void D3D11Renderer::BeginFrame() {
  m_Context->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(),
                                m_DepthStencilView.Get());
  m_Context->RSSetViewports(1, &m_Viewport);
}

void D3D11Renderer::EndFrame() {
  // Frame cleanup
}

void D3D11Renderer::Clear(f32 r, f32 g, f32 b, f32 a) {
  f32 clearColor[4] = {r, g, b, a};
  m_Context->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
  m_Context->ClearDepthStencilView(m_DepthStencilView.Get(),
                                   D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                   1.0f, 0);
}

void D3D11Renderer::Present() { m_SwapChain->Present(m_VSync ? 1 : 0, 0); }

void D3D11Renderer::DrawCube() {
  if (!m_CubeInitialized) {
    if (!InitCube()) {
      return;
    }
  }

  m_Rotation += 0.01f;

  // Matrices
  XMMATRIX world =
      XMMatrixRotationY(m_Rotation) * XMMatrixRotationX(m_Rotation * 0.5f);
  XMVECTOR eye = XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f);
  XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
  XMMATRIX projection = XMMatrixPerspectiveFovLH(
      XM_PIDIV4, m_Width / (f32)m_Height, 0.01f, 100.0f);
  XMMATRIX wvp = world * view * projection;

  // Update constant buffer
  XMFLOAT4X4 wvpData;
  XMStoreFloat4x4(&wvpData, XMMatrixTranspose(wvp));
  m_Context->UpdateSubresource(m_CubeConstantBuffer.Get(), 0, nullptr, &wvpData,
                               0, 0);

  // Set pipeline state
  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  m_Context->IASetVertexBuffers(0, 1, m_CubeVertexBuffer.GetAddressOf(),
                                &stride, &offset);
  m_Context->IASetIndexBuffer(m_CubeIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_Context->VSSetShader(m_CubeVS.Get(), nullptr, 0);
  m_Context->VSSetConstantBuffers(0, 1, m_CubeConstantBuffer.GetAddressOf());
  m_Context->PSSetShader(m_CubePS.Get(), nullptr, 0);

  m_Context->DrawIndexed(36, 0, 0);
}

void D3D11Renderer::OnResize(u32 width, u32 height) {
  if (width == 0 || height == 0 || !m_SwapChain) {
    return;
  }

  m_Width = width;
  m_Height = height;

  m_RenderTargetView.Reset();
  m_DepthStencilView.Reset();
  m_DepthStencilBuffer.Reset();

  HRESULT hr =
      m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to resize swap chain buffers");
    return;
  }

  CreateRenderTargetView();
  CreateDepthStencilView();

  m_Viewport.TopLeftX = 0.0f;
  m_Viewport.TopLeftY = 0.0f;
  m_Viewport.Width = static_cast<f32>(m_Width);
  m_Viewport.Height = static_cast<f32>(m_Height);
  m_Viewport.MinDepth = 0.0f;
  m_Viewport.MaxDepth = 1.0f;

  HORSE_LOG_RENDER_INFO("Renderer resized: {}x{}", m_Width, m_Height);
}

bool D3D11Renderer::CreateDepthStencilView() {
  D3D11_TEXTURE2D_DESC depthDesc = {};
  depthDesc.Width = m_Width;
  depthDesc.Height = m_Height;
  depthDesc.MipLevels = 1;
  depthDesc.ArraySize = 1;
  depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthDesc.SampleDesc.Count = 1;
  depthDesc.SampleDesc.Quality = 0;
  depthDesc.Usage = D3D11_USAGE_DEFAULT;
  depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

  HRESULT hr =
      m_Device->CreateTexture2D(&depthDesc, nullptr, &m_DepthStencilBuffer);
  if (FAILED(hr))
    return false;

  hr = m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr,
                                        &m_DepthStencilView);
  if (FAILED(hr))
    return false;

  return true;
}

bool D3D11Renderer::InitCube() {
  // Create shaders
  D3D11Shader vs, ps;
  if (!vs.CompileFromFile(m_Device.Get(),
                          L"Engine/Runtime/Shaders/Triangle.hlsl", "VS",
                          "vs_5_0"))
    return false;
  if (!ps.CompileFromFile(m_Device.Get(),
                          L"Engine/Runtime/Shaders/Triangle.hlsl", "PS",
                          "ps_5_0"))
    return false;

  m_CubeVS = vs.GetVertexShader();
  m_CubePS = ps.GetPixelShader();

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT hr = m_Device->CreateInputLayout(
      layout, ARRAYSIZE(layout), vs.GetBytecode()->GetBufferPointer(),
      vs.GetBytecode()->GetBufferSize(), &m_CubeInputLayout);
  if (FAILED(hr))
    return false;

  // Create vertex buffer (Rainbow Cube)
  Vertex vertices[] = {
      {-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f}, // Front-top-left (Red)
      {1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f},  // Front-top-right (Green)
      {1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f}, // Front-bottom-right (Blue)
      {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
       1.0f},                                      // Front-bottom-left (Yellow)
      {-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, // Back-top-left (Magenta)
      {1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f},  // Back-top-right (Cyan)
      {1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Back-bottom-right (White)
      {-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}, // Back-bottom-left (Black)
  };

  D3D11_BUFFER_DESC vbd = {};
  vbd.Usage = D3D11_USAGE_DEFAULT;
  vbd.ByteWidth = sizeof(vertices);
  vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vsd = {};
  vsd.pSysMem = vertices;
  hr = m_Device->CreateBuffer(&vbd, &vsd, &m_CubeVertexBuffer);
  if (FAILED(hr))
    return false;

  // Create index buffer
  u32 indices[] = {
      0, 1, 2, 0, 2, 3, // Front
      4, 5, 6, 4, 6, 7, // Back
      4, 0, 3, 4, 3, 7, // Left
      1, 5, 6, 1, 6, 2, // Right
      4, 5, 1, 4, 1, 0, // Top
      3, 2, 6, 3, 6, 7  // Bottom
  };

  D3D11_BUFFER_DESC ibd = {};
  ibd.Usage = D3D11_USAGE_DEFAULT;
  ibd.ByteWidth = sizeof(indices);
  ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA isd = {};
  isd.pSysMem = indices;
  hr = m_Device->CreateBuffer(&ibd, &isd, &m_CubeIndexBuffer);
  if (FAILED(hr))
    return false;

  // Create constant buffer
  D3D11_BUFFER_DESC cbd = {};
  cbd.Usage = D3D11_USAGE_DEFAULT;
  cbd.ByteWidth = sizeof(XMFLOAT4X4);
  cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  hr = m_Device->CreateBuffer(&cbd, nullptr, &m_CubeConstantBuffer);
  if (FAILED(hr))
    return false;

  m_CubeInitialized = true;
  return true;
}

bool D3D11Renderer::CreateDeviceAndSwapChain(const RendererDesc &desc) {
  DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
  swapChainDesc.BufferCount = 2;
  swapChainDesc.BufferDesc.Width = desc.Width;
  swapChainDesc.BufferDesc.Height = desc.Height;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.OutputWindow = desc.WindowHandle;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.SampleDesc.Quality = 0;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  UINT createDeviceFlags = 0;
  if (desc.Debug) {
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  }

  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0,
  };

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &swapChainDesc,
      &m_SwapChain, &m_Device, &m_FeatureLevel, &m_Context);

  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("D3D11CreateDeviceAndSwapChain failed: {:#x}",
                           static_cast<u32>(hr));
    return false;
  }

  HORSE_LOG_RENDER_INFO("D3D11 Device created (Feature Level: {:#x})",
                        static_cast<u32>(m_FeatureLevel));
  return true;
}

bool D3D11Renderer::CreateRenderTargetView() {
  ComPtr<ID3D11Texture2D> backBuffer;
  HRESULT hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to get back buffer");
    return false;
  }

  hr = m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                        &m_RenderTargetView);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create render target view");
    return false;
  }

  m_Context->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), nullptr);
  return true;
}

} // namespace Horse

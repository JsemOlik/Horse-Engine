#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Render/D3D11Shader.h"
#include <dxgi1_2.h>

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
  m_TriangleVertexBuffer.Reset();
  m_TriangleInputLayout.Reset();
  m_TriangleVS.Reset();
  m_TrianglePS.Reset();

  m_RenderTargetView.Reset();
  m_SwapChain.Reset();
  m_Context.Reset();
  m_Device.Reset();
}

void D3D11Renderer::BeginFrame() {
  m_Context->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), nullptr);
  m_Context->RSSetViewports(1, &m_Viewport);
}

void D3D11Renderer::EndFrame() {
  // Frame cleanup
}

void D3D11Renderer::Clear(f32 r, f32 g, f32 b, f32 a) {
  f32 clearColor[4] = {r, g, b, a};
  m_Context->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
}

void D3D11Renderer::Present() { m_SwapChain->Present(m_VSync ? 1 : 0, 0); }

void D3D11Renderer::DrawTriangle() {
  if (!m_TriangleInitialized) {
    if (!InitTriangle()) {
      return;
    }
  }

  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  m_Context->IASetVertexBuffers(0, 1, m_TriangleVertexBuffer.GetAddressOf(),
                                &stride, &offset);
  m_Context->IASetInputLayout(m_TriangleInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_Context->VSSetShader(m_TriangleVS.Get(), nullptr, 0);
  m_Context->PSSetShader(m_TrianglePS.Get(), nullptr, 0);

  m_Context->Draw(3, 0);
}

void D3D11Renderer::OnResize(u32 width, u32 height) {
  if (width == 0 || height == 0 || !m_SwapChain) {
    return;
  }

  m_Width = width;
  m_Height = height;

  m_RenderTargetView.Reset();

  HRESULT hr =
      m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to resize swap chain buffers");
    return;
  }

  CreateRenderTargetView();

  m_Viewport.TopLeftX = 0.0f;
  m_Viewport.TopLeftY = 0.0f;
  m_Viewport.Width = static_cast<f32>(m_Width);
  m_Viewport.Height = static_cast<f32>(m_Height);
  m_Viewport.MinDepth = 0.0f;
  m_Viewport.MaxDepth = 1.0f;

  HORSE_LOG_RENDER_INFO("Renderer resized: {}x{}", m_Width, m_Height);
}

bool D3D11Renderer::InitTriangle() {
  // Create shaders
  D3D11Shader vs, ps;
  if (!vs.CompileFromFile(m_Device.Get(),
                          L"Engine/Runtime/Shaders/Triangle.hlsl", "VS",
                          "vs_5_0")) {
    return false;
  }
  if (!ps.CompileFromFile(m_Device.Get(),
                          L"Engine/Runtime/Shaders/Triangle.hlsl", "PS",
                          "ps_5_0")) {
    return false;
  }

  m_TriangleVS = vs.GetVertexShader();
  m_TrianglePS = ps.GetPixelShader();

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT hr = m_Device->CreateInputLayout(
      layout, ARRAYSIZE(layout), vs.GetBytecode()->GetBufferPointer(),
      vs.GetBytecode()->GetBufferSize(), &m_TriangleInputLayout);
  if (FAILED(hr)) {
    return false;
  }

  // Create vertex buffer
  Vertex vertices[] = {
      {0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
      {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
      {-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f},
  };

  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = sizeof(vertices);
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA sd = {};
  sd.pSysMem = vertices;

  hr = m_Device->CreateBuffer(&bd, &sd, &m_TriangleVertexBuffer);
  if (FAILED(hr)) {
    return false;
  }

  m_TriangleInitialized = true;
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

#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Render/D3D11Buffer.h"
#include "HorseEngine/Render/D3D11Shader.h"
#include "HorseEngine/Render/D3D11Texture.h"
#include "HorseEngine/Render/Frustum.h"
#include "HorseEngine/Render/Material.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"
#include <DirectXMath.h>
#include <algorithm>
#include <dxgi1_2.h>

using namespace DirectX;

namespace Horse {

D3D11Renderer::D3D11Renderer() = default;

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

  // Create Material Constant Buffer
  m_MaterialConstantBuffer = std::make_unique<D3D11Buffer>();
  if (!m_MaterialConstantBuffer->Initialize(
          m_Device.Get(), BufferType::Constant, BufferUsage::Dynamic, nullptr,
          sizeof(MaterialConstantBuffer))) {
    HORSE_LOG_RENDER_ERROR("Failed to create Material Constant Buffer");
    return false;
  }

  // Create Default White Texture
  uint32_t whitePixel = 0xFFFFFFFF;
  m_WhiteTexture = std::make_unique<D3D11Texture>();
  if (!m_WhiteTexture->Create(m_Device.Get(), 1, 1, &whitePixel)) {
    HORSE_LOG_RENDER_ERROR("Failed to create default white texture");
    return false;
  }

  // Load Default Shader
  m_DefaultShader = std::make_shared<D3D11Shader>();
  if (!m_DefaultShader->CompileFromFile(m_Device.Get(),
                                        L"Engine/Runtime/Shaders/Triangle.hlsl",
                                        "VS", "vs_5_0")) {
    HORSE_LOG_RENDER_ERROR("Failed to compile default vertex shader");
    return false;
  }
  // Hack: Re-using the same object to compile pixel shader, assuming internal
  // state handles it or we need separate calls? D3D11Shader logic:
  // CompileFromFile stores GetVertexShader/GetPixelShader. Let's check
  // D3D11Shader.h again. It has both members.
  if (!m_DefaultShader->CompileFromFile(m_Device.Get(),
                                        L"Engine/Runtime/Shaders/Triangle.hlsl",
                                        "PS", "ps_5_0")) {
    HORSE_LOG_RENDER_ERROR("Failed to compile default pixel shader");
    return false;
  }

  m_Shaders["StandardPBR"] = m_DefaultShader;

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
  m_CubeTexture.reset();
  m_CubeVertexBuffer.reset();
  m_CubeIndexBuffer.reset();
  m_CubeConstantBuffer.reset();
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
  m_CubeConstantBuffer->UpdateData(m_Context.Get(), &wvpData, sizeof(wvpData));

  // Set pipeline state
  m_CubeVertexBuffer->Bind(m_Context.Get(), 0);
  m_CubeIndexBuffer->Bind(m_Context.Get(), 0);
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_Context->VSSetShader(m_CubeVS.Get(), nullptr, 0);
  m_CubeConstantBuffer->Bind(m_Context.Get(), 0);

  if (m_CubeTexture) {
    m_CubeTexture->Bind(m_Context.Get(), 0);
  }

  m_Context->PSSetShader(m_CubePS.Get(), nullptr, 0);

  m_Context->DrawIndexed(36, 0, 0);
}

void D3D11Renderer::RenderScene(Scene *scene, const XMMATRIX *overrideView,
                                const XMMATRIX *overrideProjection) {
  if (!scene)
    return;
  if (!m_CubeInitialized) {
    if (!InitCube())
      return;
  }

  entt::registry &registry = scene->GetRegistry();

  XMMATRIX view = overrideView ? *overrideView : XMMatrixIdentity();
  XMMATRIX projection =
      overrideProjection
          ? *overrideProjection
          : XMMatrixPerspectiveFovLH(XM_PIDIV4, m_Width / (f32)m_Height, 0.1f,
                                     1000.0f);
  bool cameraFound = (overrideView != nullptr && overrideProjection != nullptr);

  if (!cameraFound) {
    // Find primary camera
    auto cameraView = registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView) {
      auto [transform, camera] =
          cameraView.get<TransformComponent, CameraComponent>(entity);
      if (!camera.Primary)
        continue;

      XMVECTOR pos = XMVectorSet(transform.Position[0], transform.Position[1],
                                 transform.Position[2], 1.0f);

      // Use transform rotation
      XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
          XMConvertToRadians(transform.Rotation[0]),
          XMConvertToRadians(transform.Rotation[1]),
          XMConvertToRadians(transform.Rotation[2]));
      XMVECTOR forward =
          XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotMat);
      XMVECTOR target = pos + forward;
      XMVECTOR up =
          XMVector3TransformCoord(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotMat);

      view = XMMatrixLookAtLH(pos, target, up);

      if (camera.Type == CameraComponent::ProjectionType::Perspective) {
        projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(camera.FOV),
                                              m_Width / (f32)m_Height,
                                              camera.NearClip, camera.FarClip);
      } else {
        float orthoWidth = camera.OrthographicSize * (m_Width / (f32)m_Height);
        float orthoHeight = camera.OrthographicSize;
        projection = XMMatrixOrthographicLH(orthoWidth, orthoHeight,
                                            camera.NearClip, camera.FarClip);
      }

      cameraFound = true;
      break;
    }
  }

  if (!cameraFound) {
    // Fallback camera
    XMVECTOR eye = XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    view = XMMatrixLookAtLH(eye, at, up);
  }

  // Create Frustum
  Frustum frustum;
  frustum.Update(view * projection);

  XMVECTOR cameraPosVec = XMVectorSet(0, 0, 0, 1);
  XMMATRIX invView = XMMatrixInverse(nullptr, view);
  cameraPosVec = XMVector3TransformCoord(cameraPosVec, invView);
  XMFLOAT3 cameraPos;
  XMStoreFloat3(&cameraPos, cameraPosVec);

  // Cull and Collect
  std::vector<RenderItem> renderItems;
  auto meshView = registry.view<TransformComponent, MeshRendererComponent>();

  for (auto entity : meshView) {
    auto [transform, mesh] =
        meshView.get<TransformComponent, MeshRendererComponent>(entity);

    // Calculate Rotation Matrix
    XMMATRIX rotation =
        XMMatrixRotationRollPitchYaw(XMConvertToRadians(transform.Rotation[0]),
                                     XMConvertToRadians(transform.Rotation[1]),
                                     XMConvertToRadians(transform.Rotation[2]));

    // Calculate World Space AABB Extents
    // Transform local extents (1.0 * Scale) into world space axes
    // Note: Vertices are -1 to 1, so the half-size (extent) is 1.0.
    XMVECTOR localExtentsX =
        XMVectorSet(transform.Scale[0] * 1.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR localExtentsY =
        XMVectorSet(0.0f, transform.Scale[1] * 1.0f, 0.0f, 0.0f);
    XMVECTOR localExtentsZ =
        XMVectorSet(0.0f, 0.0f, transform.Scale[2] * 1.0f, 0.0f);

    XMVECTOR rotExtentsX = XMVector3TransformNormal(localExtentsX, rotation);
    XMVECTOR rotExtentsY = XMVector3TransformNormal(localExtentsY, rotation);
    XMVECTOR rotExtentsZ = XMVector3TransformNormal(localExtentsZ, rotation);

    // Sum absolute values to get the bounding box extents
    XMVECTOR worldExtents = XMVectorAbs(rotExtentsX) +
                            XMVectorAbs(rotExtentsY) + XMVectorAbs(rotExtentsZ);

    AABB aabb;
    aabb.Center = {transform.Position[0], transform.Position[1],
                   transform.Position[2]};
    XMStoreFloat3(&aabb.Extents, worldExtents);

    if (frustum.Intersects(aabb)) {
      float dx = transform.Position[0] - cameraPos.x;
      float dy = transform.Position[1] - cameraPos.y;
      float dz = transform.Position[2] - cameraPos.z;
      float distSq = dx * dx + dy * dy + dz * dz;

      renderItems.push_back({entity, distSq});
    }
  }

  // Sort front-to-back (optimization for opaque objects to leverage early-Z)
  std::sort(renderItems.begin(), renderItems.end(),
            [](const RenderItem &a, const RenderItem &b) {
              return a.DistanceSq < b.DistanceSq;
            });

  // Set common pipeline state
  m_CubeVertexBuffer->Bind(m_Context.Get(), 0);
  m_CubeIndexBuffer->Bind(m_Context.Get(), 0);
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_Context->VSSetShader(m_CubeVS.Get(), nullptr, 0);
  m_CubeConstantBuffer->Bind(m_Context.Get(), 0);

  if (m_CubeTexture) {
    m_CubeTexture->Bind(m_Context.Get(), 0);
  }

  m_Context->PSSetShader(m_CubePS.Get(), nullptr, 0);

  // Render Sorted Items
  for (const auto &item : renderItems) {
    auto [transform, mesh] =
        meshView.get<TransformComponent, MeshRendererComponent>(item.Entity);

    XMMATRIX world =
        XMMatrixScaling(transform.Scale[0], transform.Scale[1],
                        transform.Scale[2]) *
        XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(transform.Rotation[0]),
            XMConvertToRadians(transform.Rotation[1]),
            XMConvertToRadians(transform.Rotation[2])) *
        XMMatrixTranslation(transform.Position[0], transform.Position[1],
                            transform.Position[2]);

    XMMATRIX wvp = world * view * projection;

    // Update Object Constant Buffer (WVP)
    XMFLOAT4X4 wvpData;
    XMStoreFloat4x4(&wvpData, XMMatrixTranspose(wvp));
    m_CubeConstantBuffer->UpdateData(m_Context.Get(), &wvpData,
                                     sizeof(wvpData));
    m_CubeConstantBuffer->Bind(m_Context.Get(), 0); // Slot 0: Object CBuffer

    // Retrieve Material properties
    // For now, assuming default/fallback if no material is present or using a
    // test material Ideally: components also have a MaterialComponent or GUID
    // Let's use a temporary fallback material for now to test the pipeline
    static Material fallbackMat("Default");
    fallbackMat.SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
    fallbackMat.SetFloat("Roughness", 0.5f);
    fallbackMat.SetFloat("Metalness", 0.0f);

    // Bind Shader
    auto shader = m_Shaders[fallbackMat.GetShaderName()];
    if (!shader)
      shader = m_DefaultShader;

    if (shader) {
      m_Context->VSSetShader(shader->GetVertexShader(), nullptr, 0);
      m_Context->PSSetShader(shader->GetPixelShader(), nullptr, 0);
    }

    // Update Material Constant Buffer
    MaterialConstantBuffer matCB;
    auto color = fallbackMat.GetColor("Albedo");
    matCB.AlbedoColor = {color[0], color[1], color[2], color[3]};
    matCB.Roughness = fallbackMat.GetFloat("Roughness");
    matCB.Metalness = fallbackMat.GetFloat("Metalness");

    m_MaterialConstantBuffer->UpdateData(m_Context.Get(), &matCB,
                                         sizeof(MaterialConstantBuffer));
    m_MaterialConstantBuffer->Bind(m_Context.Get(),
                                   1); // Slot 1: Material CBuffer

    // Bind Textures
    // If material has an albedo texture, bind it. Else bind white texture.
    std::string albedoPath = fallbackMat.GetTexture("AlbedoMap");
    // TODO: Texture Manager lookup. For now, just bind the white texture if
    // nothing else Or reuse the checkerboard if we want to visualize it
    if (m_CubeTexture) {
      m_CubeTexture->Bind(m_Context.Get(), 0);
    } else {
      m_WhiteTexture->Bind(m_Context.Get(), 0);
    }

    m_Context->DrawIndexed(36, 0, 0);
  }
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
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT hr = m_Device->CreateInputLayout(
      layout, ARRAYSIZE(layout), vs.GetBytecode()->GetBufferPointer(),
      vs.GetBytecode()->GetBufferSize(), &m_CubeInputLayout);
  if (FAILED(hr))
    return false;

  // Create vertex buffer (Textured Cube - 24 vertices for unique UVs per face)
  Vertex vertices[] = {
      // Front Face
      {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

      // Back Face
      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

      // Left Face
      {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

      // Right Face
      {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

      // Top Face
      {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},

      // Bottom Face
      {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
      {1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
      {1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      {-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},
  };

  m_CubeVertexBuffer = std::make_unique<D3D11Buffer>();
  if (!m_CubeVertexBuffer->Initialize(m_Device.Get(), BufferType::Vertex,
                                      BufferUsage::Immutable, vertices,
                                      sizeof(vertices), sizeof(Vertex)))
    return false;

  // Create index buffer
  u32 indices[] = {
      0,  1,  2,  0,  2,  3,  // Front
      4,  5,  6,  4,  6,  7,  // Back
      8,  9,  10, 8,  10, 11, // Left
      12, 13, 14, 12, 14, 15, // Right
      16, 17, 18, 16, 18, 19, // Top
      20, 21, 22, 20, 22, 23, // Bottom
  };

  m_CubeIndexBuffer = std::make_unique<D3D11Buffer>();
  if (!m_CubeIndexBuffer->Initialize(m_Device.Get(), BufferType::Index,
                                     BufferUsage::Immutable, indices,
                                     sizeof(indices)))
    return false;

  // Create constant buffer
  m_CubeConstantBuffer = std::make_unique<D3D11Buffer>();
  if (!m_CubeConstantBuffer->Initialize(m_Device.Get(), BufferType::Constant,
                                        BufferUsage::Dynamic, nullptr,
                                        sizeof(XMFLOAT4X4)))
    return false;

  // Load texture
  m_CubeTexture = std::make_unique<D3D11Texture>();
  if (!m_CubeTexture->LoadFromFile(
          m_Device.Get(), m_Context.Get(),
          "Engine/Runtime/Textures/Checkerboard.png")) {
    HORSE_LOG_RENDER_WARN("Failed to load test texture, cube will be white");
  }

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

#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Render/D3D11Buffer.h"
#include "HorseEngine/Render/D3D11Shader.h"
#include "HorseEngine/Render/D3D11Texture.h"
#include "HorseEngine/Render/Frustum.h"
#include "HorseEngine/Render/Material.h"
#include "HorseEngine/Render/MaterialRegistry.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"
#include <DirectXMath.h>
#include <algorithm>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

  if (!CreateRasterizerStates()) {
    HORSE_LOG_RENDER_ERROR("Failed to create rasterizer states");
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
  m_RasterizerStateSolid.Reset();
  m_RasterizerStateWireframe.Reset();
  m_RenderTargetView.Reset();
  m_SwapChain.Reset();
  m_Context.Reset();
  m_Device.Reset();

  m_SkyboxTexture.reset();
  m_SkyboxVS.Reset();
  m_SkyboxPS.Reset();
  m_RasterizerStateSkybox.Reset();
}

void D3D11Renderer::SetViewMode(int mode) { m_ViewMode = mode; }

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

void D3D11Renderer::DrawSkybox(const DirectX::XMMATRIX &view,
                               const DirectX::XMMATRIX &projection) {
  if (!m_SkyboxVS || !m_SkyboxPS || !m_SkyboxTexture || !m_CubeInitialized)
    return;

  // Skybox follows the camera, so we zero out the translation from the view
  // matrix Or we can just build a rotation-only view matrix.
  XMMATRIX rotationView = view;
  rotationView.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // Zero translation

  // WVP for Skybox:
  // World (Identity, but scale huge?) -> No, VS sets Z=W, so standard unit cube
  // works if we want to sample direction.
  XMMATRIX wvp = rotationView * projection;

  // Update constant buffer
  XMFLOAT4X4 wvpData;
  XMStoreFloat4x4(&wvpData, XMMatrixTranspose(wvp));
  m_CubeConstantBuffer->UpdateData(m_Context.Get(), &wvpData, sizeof(wvpData));
  m_CubeConstantBuffer->Bind(m_Context.Get(), 0);

  // Set Pipeline State
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_CubeVertexBuffer->Bind(m_Context.Get(), 0);
  m_CubeIndexBuffer->Bind(m_Context.Get(), 0);

  // Rasterizer: Cull None (we are inside) or Front (if winding order matches)
  m_Context->RSSetState(m_RasterizerStateSkybox.Get());

  // Depth Stencil:
  // We want Skybox to be at Far Plane (Z=1).
  // If we draw it LAST (recommended), we need Depth Func = LESS_EQUAL (since
  // usually it's LESS). And Depth Write should be disabled to not mess with
  // existing depth buffer? Actually if it's strictly at far plane, it won't
  // overwrite closer objects. But we might want it to be behind everything.
  // Standard way: Draw Last. Depth Func: LessEqual. Depth Write: Off.
  D3D11_DEPTH_STENCIL_DESC dsDesc = {};
  dsDesc.DepthEnable = TRUE;
  dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Don't write depth
  dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Ensure it passes at Z=1
  ComPtr<ID3D11DepthStencilState> dsState;
  m_Device->CreateDepthStencilState(&dsDesc, &dsState);
  m_Context->OMSetDepthStencilState(dsState.Get(), 0);

  // Bind Shaders and Texture
  m_Context->VSSetShader(m_SkyboxVS.Get(), nullptr, 0);
  m_Context->PSSetShader(m_SkyboxPS.Get(), nullptr, 0);
  m_SkyboxTexture->Bind(m_Context.Get(), 0);

  m_Context->DrawIndexed(36, 0, 0);

  // Restore Default Depth State (Standard)
  m_Context->OMSetDepthStencilState(nullptr, 0);
  // Restore Default Rasterizer
  m_Context->RSSetState(m_RasterizerStateSolid.Get());
}

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
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_CubeVertexBuffer->Bind(m_Context.Get(), 0);
  m_CubeIndexBuffer->Bind(m_Context.Get(), 0);

  // Constants are updated in RenderScene
  // m_Context->VSSetShader(m_CubeVS.Get(), nullptr, 0);
  // m_Context->PSSetShader(m_CubePS.Get(), nullptr, 0);
  // m_Context->DrawIndexed(36, 0, 0);
}

void D3D11Renderer::DrawWireBox(const DirectX::XMMATRIX &view,
                                const DirectX::XMMATRIX &projection,
                                const DirectX::XMMATRIX &world,
                                const DirectX::XMFLOAT4 &color) {
  if (!m_WireframeIndexBuffer || !m_CubeInitialized)
    return;

  XMMATRIX wvp = world * view * projection;

  // Update constant buffer
  XMFLOAT4X4 wvpData;
  XMStoreFloat4x4(&wvpData, XMMatrixTranspose(wvp));
  m_CubeConstantBuffer->UpdateData(m_Context.Get(), &wvpData, sizeof(wvpData));
  m_CubeConstantBuffer->Bind(m_Context.Get(), 0);

  // Update Material Constant Buffer with the wireframe color
  MaterialConstantBuffer matCB;
  matCB.AlbedoColor = color;
  matCB.Roughness = 0.0f;
  matCB.Metalness = 0.0f;
  m_MaterialConstantBuffer->UpdateData(m_Context.Get(), &matCB,
                                       sizeof(MaterialConstantBuffer));
  m_MaterialConstantBuffer->Bind(m_Context.Get(), 1);

  // Use default white texture
  m_WhiteTexture->Bind(m_Context.Get(), 0);

  // Set pipeline state
  m_Context->IASetInputLayout(m_CubeInputLayout.Get());
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  m_CubeVertexBuffer->Bind(m_Context.Get(), 0);
  m_WireframeIndexBuffer->Bind(m_Context.Get(), 0);

  m_Context->VSSetShader(m_CubeVS.Get(), nullptr, 0);
  m_Context->PSSetShader(m_CubePS.Get(), nullptr, 0);

  m_Context->DrawIndexed(24, 0, 0);

  // Reset topology
  m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

std::shared_ptr<D3D11Shader>
D3D11Renderer::GetShader(const std::string &shaderName,
                         const MaterialInstance &material) {
  std::string key = shaderName;
  std::vector<D3D_SHADER_MACRO> defines;

  // Build Permutation Key & Defines
  // TODO: This mapping should ideally be data-driven or based on shader
  // metadata
  if (material.HasTexture("AlbedoMap")) {
    key += "_ALBEDO";
    defines.push_back({"HAS_ALBEDO_MAP", "1"});
  }
  // Add other properties here (NormalMap, RoughnessMap, etc.)

  // Check Cache
  auto it = m_Shaders.find(key);
  if (it != m_Shaders.end()) {
    return it->second;
  }

  // Compile New Variant
  HORSE_LOG_RENDER_INFO("Compiling Shader Variant: {}", key);
  auto shader = std::make_shared<D3D11Shader>();

  // Resolve Path
  // Hardcoded mapping for now
  std::wstring path = L"Engine/Runtime/Shaders/Triangle.hlsl";
  if (shaderName != "StandardPBR") {
    // Fallback or specific paths
  }

  // Compile VS and PS (Assume same file for now)
  // IMPORTANT: In production, Vertex Shader might not need the same
  // permutations as Pixel Shader, or might need different ones (e.g. skinning).
  // Current simple system assumes shared logic.
  bool success = true;
  if (!shader->CompileFromFile(m_Device.Get(), path, "VS", "vs_5_0", defines))
    success = false;
  if (!shader->CompileFromFile(m_Device.Get(), path, "PS", "ps_5_0", defines))
    success = false;

  if (!success) {
    HORSE_LOG_RENDER_ERROR("Failed to compile shader variant: {}", key);
    return m_DefaultShader;
  }

  m_Shaders[key] = shader;
  return shader;
}

void D3D11Renderer::RenderScene(Scene *scene, const XMMATRIX *overrideView,
                                const XMMATRIX *overrideProjection,
                                uint32_t viewportWidth,
                                uint32_t viewportHeight) {
  if (!scene)
    return;
  if (!m_CubeInitialized) {
    if (!InitCube())
      return;
  }

  uint32_t width = viewportWidth > 0 ? viewportWidth : m_Width;
  uint32_t height = viewportHeight > 0 ? viewportHeight : m_Height;
  float aspectRatio = (float)width / (float)height;

  entt::registry &registry = scene->GetRegistry();

  XMMATRIX view = overrideView ? *overrideView : XMMatrixIdentity();
  XMMATRIX projection =
      overrideProjection
          ? *overrideProjection
          : XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 1000.0f);
  bool cameraFound = (overrideView != nullptr && overrideProjection != nullptr);

  entt::entity cameraOwner = entt::null; // Track who owns the camera

  if (!cameraFound) {
    // Find primary camera
    auto cameraView = registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView) {
      auto [transform, camera] =
          cameraView.get<TransformComponent, CameraComponent>(entity);
      if (!camera.Primary)
        continue;

      // Use WorldTransform for correct hierarchy support
      glm::mat4 worldTransform = transform.WorldTransform;

      // Capture Camera Parent (The Player Body)
      Horse::Entity camEnt(entity, scene);
      if (camEnt.HasComponent<RelationshipComponent>()) {
        cameraOwner = camEnt.GetComponent<RelationshipComponent>().Parent;
      }

      // Need to duplicate the matrix setup code here or trust the previous
      // implementation. For safety in this Replace Block, I'll assume the
      // context is inside the camera loop. Reuse the logic I wrote before:

      // Convert glm::mat4 to XMMATRIX
      XMMATRIX worldMat = XMMatrixSet(
          worldTransform[0][0], worldTransform[0][1], worldTransform[0][2],
          worldTransform[0][3], worldTransform[1][0], worldTransform[1][1],
          worldTransform[1][2], worldTransform[1][3], worldTransform[2][0],
          worldTransform[2][1], worldTransform[2][2], worldTransform[2][3],
          worldTransform[3][0], worldTransform[3][1], worldTransform[3][2],
          worldTransform[3][3]);

      // View Matrix is Inverse of World Matrix
      XMVECTOR det;
      view = XMMatrixInverse(&det, worldMat);

      if (camera.Type == CameraComponent::ProjectionType::Perspective) {
        projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(camera.FOV),
                                              aspectRatio, camera.NearClip,
                                              camera.FarClip);
      } else {
        float orthoWidth = camera.OrthographicSize * aspectRatio;
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
    // Hidden From Player Camera Logic:
    // If this entity owns the current camera (i.e., is the Player Body), do not
    // render it.
    if (entity == cameraOwner)
      continue;

    auto [transform, mesh] =
        meshView.get<TransformComponent, MeshRendererComponent>(entity);

    // Calculate Rotation Matrix from WorldTransform decomposition or directly
    // use rows/cols Since WorldTransform (GLM) is Col-Major, it matches DirectX
    // Row-Major memory layout. XMMATRIX world = S * R * T. The columns of GLM
    // (rows of DX) are the basis vectors * scale.

    // Load World Matrix
    XMMATRIX worldMat = DirectX::XMLoadFloat4x4(
        (const DirectX::XMFLOAT4X4 *)glm::value_ptr(transform.WorldTransform));

    // Calculate World Space AABB Extents
    // Start with local extents of 1.0 (assuming unit cube/mesh)
    XMVECTOR dirX =
        worldMat.r[0]; // (ScaleX * RightX, ScaleX * RightY, ScaleX * RightZ)
    XMVECTOR dirY = worldMat.r[1];
    XMVECTOR dirZ = worldMat.r[2];

    // Sum absolute values to get the bounding box extents
    XMVECTOR worldExtents =
        XMVectorAbs(dirX) + XMVectorAbs(dirY) + XMVectorAbs(dirZ);

    AABB aabb;
    // Position is at the last row (3)
    XMStoreFloat3(&aabb.Center, worldMat.r[3]);
    XMStoreFloat3(&aabb.Extents, worldExtents);

    if (frustum.Intersects(aabb)) {
      float dx = transform.Position[0] - cameraPos.x;
      float dy = transform.Position[1] - cameraPos.y;
      float dz = transform.Position[2] - cameraPos.z;
      float distSq = dx * dx + dy * dy + dz * dz;

      renderItems.push_back({entity, distSq});
    }
  }

  // Draw Skybox Last (Background)
  // Only if we found a camera
  if (cameraFound) {
    DrawSkybox(view, projection);
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

  // Set Rasterizer State
  // Wireframe state only for mode 1. Mode 2 (Colored Triangles) uses Solid
  // state
  m_Context->RSSetState(m_ViewMode == 1 ? m_RasterizerStateWireframe.Get()
                                        : m_RasterizerStateSolid.Get());

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

    XMMATRIX world = DirectX::XMLoadFloat4x4(
        (const DirectX::XMFLOAT4X4 *)glm::value_ptr(transform.WorldTransform));

    XMMATRIX wvp = world * view * projection;

    // Update Object Constant Buffer (WVP)
    XMFLOAT4X4 wvpData;
    XMStoreFloat4x4(&wvpData, XMMatrixTranspose(wvp));
    m_CubeConstantBuffer->UpdateData(m_Context.Get(), &wvpData,
                                     sizeof(wvpData));
    m_CubeConstantBuffer->Bind(m_Context.Get(), 0); // Slot 0: Object CBuffer

    // Retrieve Material properties
    // Use Material Registry to get the material for this entity
    auto material = MaterialRegistry::Get().GetMaterial(mesh.MaterialGUID);
    if (!material) {
      material = MaterialRegistry::Get().GetMaterial("Default");
    }

    // Bind Shader (Permutation Aware)
    auto shader = GetShader(material->GetShaderName(), *material);
    if (!shader)
      shader = m_DefaultShader;

    if (shader) {
      m_Context->VSSetShader(shader->GetVertexShader(), nullptr, 0);
      m_Context->PSSetShader(shader->GetPixelShader(), nullptr, 0);
    }

    // Update Material Constant Buffer
    MaterialConstantBuffer matCB;
    auto color = material->GetColor("Albedo");
    matCB.AlbedoColor = {color[0], color[1], color[2], color[3]};
    matCB.Roughness = material->GetFloat("Roughness");
    matCB.Metalness = material->GetFloat("Metalness");
    matCB.ViewMode = m_ViewMode;

    m_MaterialConstantBuffer->UpdateData(m_Context.Get(), &matCB,
                                         sizeof(MaterialConstantBuffer));
    m_MaterialConstantBuffer->Bind(m_Context.Get(),
                                   1); // Slot 1: Material CBuffer

    // Bind Textures
    // If material has an albedo texture, bind it. Else bind white texture.
    std::string albedoPath = material->GetTexture("AlbedoMap");

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

  // Create wireframe index buffer (12 lines * 2 indices = 24)
  u32 wireIndices[] = {
      0, 1, 1, 2, 2, 3, 3, 0, // Front
      4, 5, 5, 6, 6, 7, 7, 4, // Back
      0, 4, 1, 5, 2, 6, 3, 7  // Side connections
  };

  m_WireframeIndexBuffer = std::make_unique<D3D11Buffer>();
  if (!m_WireframeIndexBuffer->Initialize(m_Device.Get(), BufferType::Index,
                                          BufferUsage::Immutable, wireIndices,
                                          sizeof(wireIndices)))
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

  // Initialize Skybox Shader
  D3D11Shader skyVS, skyPS;
  if (skyVS.CompileFromFile(m_Device.Get(),
                            L"Engine/Runtime/Shaders/Skybox.hlsl", "VS",
                            "vs_5_0")) {
    m_SkyboxVS = skyVS.GetVertexShader();
  } else {
    HORSE_LOG_RENDER_ERROR("Failed to compile Skybox VS");
  }

  if (skyPS.CompileFromFile(m_Device.Get(),
                            L"Engine/Runtime/Shaders/Skybox.hlsl", "PS",
                            "ps_5_0")) {
    m_SkyboxPS = skyPS.GetPixelShader();
  } else {
    HORSE_LOG_RENDER_ERROR("Failed to compile Skybox PS");
  }

  // Load Skybox Texture
  m_SkyboxTexture = std::make_unique<D3D11Texture>();
  if (!m_SkyboxTexture->LoadFromFile(m_Device.Get(), m_Context.Get(),
                                     "Engine/Runtime/Textures/Skybox.png")) {
    HORSE_LOG_RENDER_WARN("Failed to load Skybox.png, sky will be empty");
  }

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
  swapChainDesc.OutputWindow = (HWND)desc.WindowHandle;
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

bool D3D11Renderer::CreateRasterizerStates() {
  D3D11_RASTERIZER_DESC desc = {};
  desc.FillMode = D3D11_FILL_SOLID;
  desc.CullMode = D3D11_CULL_BACK;
  desc.FrontCounterClockwise = FALSE;
  desc.DepthBias = 0;
  desc.DepthBiasClamp = 0.0f;
  desc.SlopeScaledDepthBias = 0.0f;
  desc.DepthClipEnable = TRUE;
  desc.ScissorEnable = FALSE;
  desc.MultisampleEnable = FALSE;
  desc.AntialiasedLineEnable = FALSE;

  HRESULT hr = m_Device->CreateRasterizerState(&desc, &m_RasterizerStateSolid);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create solid rasterizer state");
    return false;
  }

  desc.FillMode = D3D11_FILL_WIREFRAME;
  desc.CullMode = D3D11_CULL_NONE; // Disable culling for wireframe
  hr = m_Device->CreateRasterizerState(&desc, &m_RasterizerStateWireframe);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create wireframe rasterizer state");
    return false;
  }

  desc.FillMode = D3D11_FILL_SOLID;
  desc.CullMode = D3D11_CULL_NONE; // Skybox needs inside faces
  hr = m_Device->CreateRasterizerState(&desc, &m_RasterizerStateSkybox);
  if (FAILED(hr)) {
    HORSE_LOG_RENDER_ERROR("Failed to create skybox rasterizer state");
    return false;
  }

  return true;
}

} // namespace Horse

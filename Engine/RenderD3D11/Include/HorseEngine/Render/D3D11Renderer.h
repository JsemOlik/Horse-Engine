#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Render/D3D11Shader.h" // Add this
#include "HorseEngine/Render/D3D11Texture.h"
#include "HorseEngine/Render/Frustum.h"
#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi.h>
#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include "HorseEngine/Render/Renderer.h"

namespace Horse {

using Microsoft::WRL::ComPtr;

class Window;
class MaterialInstance;

struct RenderItem {
  entt::entity Entity;
  float DistanceSq; // To camera
};

class D3D11Renderer : public Renderer {
public:
  D3D11Renderer();
  virtual ~D3D11Renderer();

  virtual bool Initialize(const RendererDesc &desc) override;
  virtual void Shutdown() override;

  virtual void BeginFrame() override;
  virtual void EndFrame() override;

  void Clear(f32 r, f32 g, f32 b, f32 a = 1.0f);
  void Present();
  virtual void
  RenderScene(class Scene *scene,
              const DirectX::XMMATRIX *overrideView = nullptr,
              const DirectX::XMMATRIX *overrideProjection = nullptr) override;

  virtual void OnResize(u32 width, u32 height) override;

  void DrawCube();
  void DrawWireBox(const DirectX::XMMATRIX &view,
                   const DirectX::XMMATRIX &projection,
                   const DirectX::XMMATRIX &world,
                   const DirectX::XMFLOAT4 &color);

  void DrawSkybox(const DirectX::XMMATRIX &view,
                  const DirectX::XMMATRIX &projection);

  ID3D11Device *GetDevice() const { return m_Device.Get(); }
  ID3D11DeviceContext *GetContext() const { return m_Context.Get(); }

  void SetViewMode(int mode); // 0=Solid, 1=Wireframe, 2=ColoredTriangles

private:
  struct Vertex {
    f32 x, y, z;
    f32 r, g, b, a;
    f32 u, v;
  };

  bool CreateDeviceAndSwapChain(const RendererDesc &desc);
  bool CreateRenderTargetView();
  bool CreateDepthStencilView();
  bool CreateRasterizerStates();
  bool InitCube();

  ComPtr<ID3D11Device> m_Device;
  ComPtr<ID3D11DeviceContext> m_Context;
  ComPtr<IDXGISwapChain> m_SwapChain;
  ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
  ComPtr<ID3D11DepthStencilView> m_DepthStencilView;
  ComPtr<ID3D11Texture2D> m_DepthStencilBuffer;

  ComPtr<ID3D11RasterizerState> m_RasterizerStateSolid;
  ComPtr<ID3D11RasterizerState> m_RasterizerStateWireframe;
  ComPtr<ID3D11RasterizerState> m_RasterizerStateSkybox; // No culling

  // Skybox Resources
  std::unique_ptr<D3D11Texture> m_SkyboxTexture;
  ComPtr<ID3D11VertexShader> m_SkyboxVS;
  ComPtr<ID3D11PixelShader> m_SkyboxPS;
  int m_ViewMode = 0;

  // Cube members
  std::unique_ptr<class D3D11Texture> m_CubeTexture;
  std::unique_ptr<class D3D11Buffer> m_CubeVertexBuffer;
  std::unique_ptr<class D3D11Buffer> m_CubeIndexBuffer;
  std::unique_ptr<class D3D11Buffer> m_WireframeIndexBuffer;
  std::unique_ptr<class D3D11Buffer> m_CubeConstantBuffer;
  ComPtr<ID3D11InputLayout> m_CubeInputLayout;
  ComPtr<ID3D11VertexShader> m_CubeVS;
  ComPtr<ID3D11PixelShader> m_CubePS;
  bool m_CubeInitialized = false;
  f32 m_Rotation = 0.0f;

  D3D11_VIEWPORT m_Viewport = {};
  D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
  u32 m_Width = 0;
  u32 m_Height = 0;
  bool m_VSync = true;

  // Material constants
  struct MaterialConstantBuffer {
    DirectX::XMFLOAT4 AlbedoColor;
    float Roughness;
    float Metalness;
    int ViewMode;
    float Padding;
  };

  std::unique_ptr<class D3D11Buffer> m_MaterialConstantBuffer;
  std::unordered_map<std::string, std::shared_ptr<class D3D11Shader>> m_Shaders;
  std::shared_ptr<class D3D11Shader> m_DefaultShader;

  std::shared_ptr<class D3D11Shader>
  GetShader(const std::string &shaderName, const MaterialInstance &material);

  // Default white texture for when no texture is bound
  std::shared_ptr<class D3D11Texture> m_WhiteTexture;
};

} // namespace Horse

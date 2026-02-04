#pragma once

#include "HorseEngine/Core.h"
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace Horse {

using Microsoft::WRL::ComPtr;

class Window;

struct RendererDesc {
    HWND WindowHandle = nullptr;
    u32 Width = 1280;
    u32 Height = 720;
    bool VSync = true;
    bool Debug = true;
};

class D3D11Renderer {
public:
    D3D11Renderer() = default;
    ~D3D11Renderer();
    
    bool Initialize(const RendererDesc& desc);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void Clear(f32 r, f32 g, f32 b, f32 a = 1.0f);
    void Present();
    
    void OnResize(u32 width, u32 height);
    
    ID3D11Device* GetDevice() const { return m_Device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_Context.Get(); }
    
private:
    bool CreateDeviceAndSwapChain(const RendererDesc& desc);
    bool CreateRenderTargetView();
    
    ComPtr<ID3D11Device> m_Device;
    ComPtr<ID3D11DeviceContext> m_Context;
    ComPtr<IDXGISwapChain> m_SwapChain;
    ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
    
    D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
    u32 m_Width = 0;
    u32 m_Height = 0;
    bool m_VSync = true;
};

} // namespace Horse

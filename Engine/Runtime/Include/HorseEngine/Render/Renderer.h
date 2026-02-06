#pragma once

#include "HorseEngine/Core.h"
#include <DirectXMath.h>

namespace Horse {

class Scene;

struct RendererDesc {
  void *WindowHandle = nullptr;
  u32 Width = 1280;
  u32 Height = 720;
  bool VSync = true;
  bool Debug = true;
};

class Renderer {
public:
  virtual ~Renderer() = default;

  virtual bool Initialize(const RendererDesc &desc) = 0;
  virtual void Shutdown() = 0;

  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;

  virtual void Clear(float r, float g, float b, float a = 1.0f) = 0;
  virtual void Present() = 0;

  virtual void
  RenderScene(Scene *scene, const DirectX::XMMATRIX *overrideView = nullptr,
              const DirectX::XMMATRIX *overrideProjection = nullptr) = 0;

  virtual void OnResize(u32 width, u32 height) = 0;
};

} // namespace Horse

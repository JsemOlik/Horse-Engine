#include "GameViewport.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Render/D3D11Renderer.h"

GameViewport::GameViewport(QWidget *parent) : D3D11ViewportWidget(parent) {}

void GameViewport::Render() {
  m_Renderer->BeginFrame();
  m_Renderer->Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2]);

  if (m_Scene) {
    m_Renderer->RenderScene(m_Scene.get(), nullptr, nullptr, width(), height());
  }

  m_Renderer->EndFrame();
  m_Renderer->Present();
}

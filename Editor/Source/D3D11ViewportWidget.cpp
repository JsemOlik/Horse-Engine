#include "D3D11ViewportWidget.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Render/D3D11Renderer.h"

#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Scene/Scene.h"
#include "QtInputMap.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>

#ifdef _WIN32
#include <Windows.h>
#endif

D3D11ViewportWidget::D3D11ViewportWidget(QWidget *parent) : QWidget(parent) {

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);

  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this]() { update(); });
  timer->start(16); // ~60 FPS
}

D3D11ViewportWidget::~D3D11ViewportWidget() {}

void D3D11ViewportWidget::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  if (!m_Initialized) {
    InitializeRenderer();
  }
}

void D3D11ViewportWidget::InitializeRenderer() {
  m_Renderer = std::make_unique<Horse::D3D11Renderer>();

  Horse::RendererDesc desc;
  desc.WindowHandle = reinterpret_cast<HWND>(winId());
  desc.Width = width();
  desc.Height = height();
  desc.VSync = true;
  desc.Debug = true;

  if (m_Renderer->Initialize(desc)) {
    m_Initialized = true;
  }
}

void D3D11ViewportWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  if (m_Initialized && m_Renderer) {
    m_Renderer->OnResize(event->size().width(), event->size().height());
  }
}

void D3D11ViewportWidget::paintEvent(QPaintEvent *event) {
  if (m_Initialized && m_Renderer) {
    Render();
  }
}

void D3D11ViewportWidget::Render() {
  Horse::Time::Update();

  m_Renderer->BeginFrame();
  m_Renderer->Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2]);

  if (m_Scene) {
    m_Renderer->RenderScene(m_Scene.get());
  }

  m_Renderer->EndFrame();
  m_Renderer->Present();
}

void D3D11ViewportWidget::keyPressEvent(QKeyEvent *event) {
  int horseKey = Horse::QtKeyToHorseKey(event->key());
  HORSE_LOG_CORE_INFO("Editor: Key Pressed: {} (Mapped to Horse: {})",
                      event->key(), horseKey);
  if (horseKey != 0) {
    Horse::Input::UpdateKeyState(horseKey, true);
  }
}

void D3D11ViewportWidget::keyReleaseEvent(QKeyEvent *event) {
  int horseKey = Horse::QtKeyToHorseKey(event->key());
  HORSE_LOG_CORE_INFO("Editor: Key Released: {} (Mapped to Horse: {})",
                      event->key(), horseKey);
  if (horseKey != 0) {
    Horse::Input::UpdateKeyState(horseKey, false);
  }
}

void D3D11ViewportWidget::mousePressEvent(QMouseEvent *event) {
  setFocus(); // Force focus on click
  HORSE_LOG_CORE_INFO("Editor: Mouse Pressed: {} (Focus Gained)",
                      (int)event->button());
  if (event->button() == Qt::LeftButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_LBUTTON, true);
  else if (event->button() == Qt::RightButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_RBUTTON, true);
  else if (event->button() == Qt::MiddleButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_MBUTTON, true);
}

void D3D11ViewportWidget::mouseReleaseEvent(QMouseEvent *event) {
  HORSE_LOG_CORE_INFO("Editor: Mouse Released: {}", (int)event->button());
  if (event->button() == Qt::LeftButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_LBUTTON, false);
  else if (event->button() == Qt::RightButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_RBUTTON, false);
  else if (event->button() == Qt::MiddleButton)
    Horse::Input::UpdateMouseButtonState(Horse::KEY_MBUTTON, false);
}

void D3D11ViewportWidget::mouseMoveEvent(QMouseEvent *event) {
  Horse::Input::UpdateMousePosition(static_cast<float>(event->pos().x()),
                                    static_cast<float>(event->pos().y()));
}

void D3D11ViewportWidget::focusInEvent(QFocusEvent *event) {
  HORSE_LOG_CORE_INFO("Editor: Viewport Focus IN");
}

void D3D11ViewportWidget::focusOutEvent(QFocusEvent *event) {
  HORSE_LOG_CORE_INFO("Editor: Viewport Focus OUT");
}

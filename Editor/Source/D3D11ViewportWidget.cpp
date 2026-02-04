#include "D3D11ViewportWidget.h"
#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Core/Time.h"

#include <QResizeEvent>
#include <QPaintEvent>
#include <QTimer>

#ifdef _WIN32
#include <Windows.h>
#endif

D3D11ViewportWidget::D3D11ViewportWidget(QWidget* parent)
    : QWidget(parent) {
    
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { update(); });
    timer->start(16); // ~60 FPS
}

D3D11ViewportWidget::~D3D11ViewportWidget() {
}

void D3D11ViewportWidget::showEvent(QShowEvent* event) {
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

void D3D11ViewportWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    if (m_Initialized && m_Renderer) {
        m_Renderer->OnResize(event->size().width(), event->size().height());
    }
}

void D3D11ViewportWidget::paintEvent(QPaintEvent* event) {
    if (m_Initialized && m_Renderer) {
        Render();
    }
}

void D3D11ViewportWidget::Render() {
    Horse::Time::Update();
    
    m_Renderer->BeginFrame();
    m_Renderer->Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2]);
    
    // Render scene here
    
    m_Renderer->EndFrame();
    m_Renderer->Present();
}

#include "SceneViewport.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Render/D3D11Renderer.h"
#include <QKeyEvent>
#include <QMouseEvent>

using namespace DirectX;

SceneViewport::SceneViewport(QWidget *parent) : D3D11ViewportWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
}

void SceneViewport::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::RightButton) {
    m_RightClickDown = true;
    m_LastMousePos = event->pos();
    setCursor(Qt::BlankCursor);
  }
}

void SceneViewport::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::RightButton) {
    m_RightClickDown = false;
    setCursor(Qt::ArrowCursor);
  }
}

void SceneViewport::mouseMoveEvent(QMouseEvent *event) {
  if (m_RightClickDown) {
    QPoint delta = event->pos() - m_LastMousePos;
    m_LastMousePos = event->pos();

    m_Yaw += delta.x() * m_LookSensitivity;
    m_Pitch += delta.y() * m_LookSensitivity;

    // Clamp pitch
    if (m_Pitch > 89.0f)
      m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
      m_Pitch = -89.0f;
  }
}

void SceneViewport::keyPressEvent(QKeyEvent *event) {
  m_KeysDown.insert(event->key());
}

void SceneViewport::keyReleaseEvent(QKeyEvent *event) {
  m_KeysDown.erase(event->key());
}

void SceneViewport::UpdateCamera() {
  if (!m_RightClickDown)
    return;

  float speed = m_MoveSpeed;
  if (m_KeysDown.count(Qt::Key_Shift))
    speed *= 2.0f;

  XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
      XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
  XMVECTOR forward =
      XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
  XMVECTOR right =
      XMVector3TransformCoord(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMVECTOR pos = XMLoadFloat3(&m_CameraPos);

  if (m_KeysDown.count(Qt::Key_W))
    pos += forward * speed;
  if (m_KeysDown.count(Qt::Key_S))
    pos -= forward * speed;
  if (m_KeysDown.count(Qt::Key_A))
    pos -= right * speed;
  if (m_KeysDown.count(Qt::Key_D))
    pos += right * speed;
  if (m_KeysDown.count(Qt::Key_E))
    pos += up * speed;
  if (m_KeysDown.count(Qt::Key_Q))
    pos -= up * speed;

  XMStoreFloat3(&m_CameraPos, pos);
}

void SceneViewport::Render() {
  UpdateCamera();

  Horse::Time::Update();

  m_Renderer->BeginFrame();
  m_Renderer->Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2]);

  if (m_Scene) {
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
    XMVECTOR forward =
        XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    XMVECTOR pos = XMLoadFloat3(&m_CameraPos);

    XMMATRIX view = XMMatrixLookAtLH(pos, pos + forward,
                                     XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX projection = XMMatrixPerspectiveFovLH(
        XM_PIDIV4, width() / (float)height(), 0.1f, 1000.0f);

    m_Renderer->RenderScene(m_Scene.get(), &view, &projection);
  }

  m_Renderer->EndFrame();
  m_Renderer->Present();
}

#include "SceneViewport.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Render/D3D11Renderer.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"
#include <DirectXMath.h>
#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QUrl>
#include <glm/gtc/type_ptr.hpp>

SceneViewport::SceneViewport(QWidget *parent) : D3D11ViewportWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setAcceptDrops(true);

  QPushButton *viewBtn = new QPushButton("View", this);
  viewBtn->setGeometry(10, 10, 80, 30);

  // Pill shape styling with dropdown arrow
  viewBtn->setStyleSheet(
      "QPushButton { "
      "  background-color: #333; "
      "  color: white; "
      "  border: 1px solid #555; "
      "  border-radius: 15px; " // Pill shape (half of height 30)
      "  padding-left: 10px; "
      "  padding-right: 10px; "
      "  text-align: center; "
      "}"
      "QPushButton::menu-indicator { image: none; }" // Hide default indicator
                                                     // if preferred, or keep it
      "QPushButton:hover { background-color: #444; }"
      "QPushButton:pressed { background-color: #222; }"
      "QMenu { "
      "  background-color: #333; "
      "  color: white; "
      "  border: 1px solid #555; "
      "}"
      "QMenu::item:selected { background-color: #444; }");

  QMenu *viewMenu = new QMenu(viewBtn);

  QAction *solidAction = viewMenu->addAction("Solid");
  QAction *wireframeAction = viewMenu->addAction("Wireframe");

  viewBtn->setMenu(viewMenu);

  connect(solidAction, &QAction::triggered, [this]() {
    if (m_Renderer) {
      m_Renderer->SetWireframe(false);
      update();
    }
  });

  connect(wireframeAction, &QAction::triggered, [this]() {
    if (m_Renderer) {
      m_Renderer->SetWireframe(true);
      update();
    }
  });
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

  DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
      DirectX::XMConvertToRadians(m_Pitch), DirectX::XMConvertToRadians(m_Yaw),
      0.0f);
  DirectX::XMVECTOR forward = DirectX::XMVector3TransformCoord(
      DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
  DirectX::XMVECTOR right = DirectX::XMVector3TransformCoord(
      DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
  DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&m_CameraPos);

  if (m_KeysDown.count(Qt::Key_W))
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(forward, speed));
  if (m_KeysDown.count(Qt::Key_S))
    pos =
        DirectX::XMVectorSubtract(pos, DirectX::XMVectorScale(forward, speed));
  if (m_KeysDown.count(Qt::Key_A))
    pos = DirectX::XMVectorSubtract(pos, DirectX::XMVectorScale(right, speed));
  if (m_KeysDown.count(Qt::Key_D))
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(right, speed));
  if (m_KeysDown.count(Qt::Key_E))
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(up, speed));
  if (m_KeysDown.count(Qt::Key_Q))
    pos = DirectX::XMVectorSubtract(pos, DirectX::XMVectorScale(up, speed));

  DirectX::XMStoreFloat3(&m_CameraPos, pos);
}

void SceneViewport::Render() {
  UpdateCamera();

  m_Renderer->BeginFrame();
  m_Renderer->Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2]);

  if (m_Scene) {

    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
        DirectX::XMConvertToRadians(m_Pitch),
        DirectX::XMConvertToRadians(m_Yaw), 0.0f);
    DirectX::XMVECTOR forward = DirectX::XMVector3TransformCoord(
        DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&m_CameraPos);

    DirectX::XMMATRIX view =
        DirectX::XMMatrixLookAtLH(pos, DirectX::XMVectorAdd(pos, forward),
                                  DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XM_PIDIV4, width() / (float)height(), 0.1f, 1000.0f);

    m_Renderer->RenderScene(m_Scene.get(), &view, &projection);

    // Draw Collider Outline for Selected Entity
    if (m_SelectedEntity &&
        m_SelectedEntity.HasComponent<Horse::BoxColliderComponent>()) {
      auto &bc = m_SelectedEntity.GetComponent<Horse::BoxColliderComponent>();
      auto &transform =
          m_SelectedEntity.GetComponent<Horse::TransformComponent>();

      // Load World Matrix
      using namespace DirectX;
      XMMATRIX worldMat = XMLoadFloat4x4(
          (const XMFLOAT4X4 *)glm::value_ptr(transform.WorldTransform));

      // Apply Box Collider Offset and Scale (Size)
      XMMATRIX boxTransform =
          XMMatrixScaling(bc.Size[0] * 0.5f, bc.Size[1] * 0.5f,
                          bc.Size[2] * 0.5f) *
          XMMatrixTranslation(bc.Offset[0], bc.Offset[1], bc.Offset[2]) *
          worldMat;

      m_Renderer->DrawWireBox(view, projection, boxTransform,
                              {0.0f, 1.0f, 0.0f, 1.0f}); // Green
    }
  }

  m_Renderer->EndFrame();
  m_Renderer->Present();
}
void SceneViewport::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void SceneViewport::dropEvent(QDropEvent *event) {
  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    for (const QUrl &url : mimeData->urls()) {
      std::filesystem::path path = url.toLocalFile().toStdString();
      auto &metadata = Horse::AssetManager::Get().GetMetadata(path);

      if (metadata.IsValid()) {
        if (metadata.Type == Horse::AssetType::Mesh) {
          // Instantiate mesh
          auto entity = m_Scene->CreateEntity(path.stem().string());
          auto &renderer = entity.AddComponent<Horse::MeshRendererComponent>();
          renderer.MeshGUID = std::to_string((uint64_t)metadata.Handle);
          HORSE_LOG_CORE_INFO("Dropped mesh: {0}", path.string());
        }
        // TODO: Handle Materials (apply to selected)
      }
    }
    event->acceptProposedAction();
  }
}

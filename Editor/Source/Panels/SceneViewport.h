#pragma once

#include "../D3D11ViewportWidget.h"
#include <DirectXMath.h>
#include <set>

class SceneViewport : public D3D11ViewportWidget {
  Q_OBJECT

public:
  explicit SceneViewport(QWidget *parent = nullptr);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

  void Render() override;

private:
  void UpdateCamera();

  DirectX::XMFLOAT3 m_CameraPos = {0.0f, 2.0f, -5.0f};
  float m_Yaw = 0.0f;
  float m_Pitch = 0.0f;

  bool m_RightClickDown = false;
  QPoint m_LastMousePos;

  std::set<int> m_KeysDown;

  float m_MoveSpeed = 0.1f;
  float m_LookSensitivity = 0.1f;
};

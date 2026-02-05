#pragma once

#include <QWidget>
#include <memory>

namespace Horse {
class D3D11Renderer;
class Scene;
} // namespace Horse

class D3D11ViewportWidget : public QWidget {
  Q_OBJECT

public:
  explicit D3D11ViewportWidget(QWidget *parent = nullptr);
  ~D3D11ViewportWidget();

  void SetScene(std::shared_ptr<Horse::Scene> scene) { m_Scene = scene; }

  QPaintEngine *paintEngine() const override { return nullptr; }

protected:
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

  virtual void Render();
  void InitializeRenderer();

  std::unique_ptr<Horse::D3D11Renderer> m_Renderer;
  std::shared_ptr<Horse::Scene> m_Scene;
  bool m_Initialized = false;
  float m_ClearColor[3] = {0.2f, 0.3f, 0.4f};
};

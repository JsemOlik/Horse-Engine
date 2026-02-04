#pragma once

#include <QWidget>
#include <memory>

namespace Horse {
    class D3D11Renderer;
}

class D3D11ViewportWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit D3D11ViewportWidget(QWidget* parent = nullptr);
    ~D3D11ViewportWidget();
    
    QPaintEngine* paintEngine() const override { return nullptr; }
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    
private:
    void InitializeRenderer();
    void Render();
    
    std::unique_ptr<Horse::D3D11Renderer> m_Renderer;
    bool m_Initialized = false;
    float m_ClearColor[3] = {0.2f, 0.3f, 0.4f};
};

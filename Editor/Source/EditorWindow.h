#pragma once

#include <QMainWindow>
#include <QDockWidget>

namespace Horse {
    class D3D11Renderer;
    class Window;
}

class D3D11ViewportWidget;
class HierarchyPanel;
class InspectorPanel;
class ContentBrowserPanel;
class ConsolePanel;

class EditorWindow : public QMainWindow {
    Q_OBJECT
    
public:
    EditorWindow(QWidget* parent = nullptr);
    ~EditorWindow();
    
private slots:
    void OnNewScene();
    void OnOpenScene();
    void OnSaveScene();
    void OnExit();
    
private:
    void CreateMenus();
    void CreatePanels();
    void CreateToolBar();
    
    D3D11ViewportWidget* m_ViewportWidget = nullptr;
    HierarchyPanel* m_HierarchyPanel = nullptr;
    InspectorPanel* m_InspectorPanel = nullptr;
    ContentBrowserPanel* m_ContentBrowserPanel = nullptr;
    ConsolePanel* m_ConsolePanel = nullptr;
};

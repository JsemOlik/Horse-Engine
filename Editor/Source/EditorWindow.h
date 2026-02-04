#pragma once

#include <QDockWidget>
#include <QMainWindow>
#include <memory>

#include "HorseEngine/Scene/Entity.h"

namespace Horse {
class D3D11Renderer;
class Window;
class Scene;
} // namespace Horse

class D3D11ViewportWidget;
class HierarchyPanel;
class InspectorPanel;
class ContentBrowserPanel;
class ConsolePanel;

class EditorWindow : public QMainWindow {
  Q_OBJECT

public:
  EditorWindow(QWidget *parent = nullptr);
  ~EditorWindow();

  std::shared_ptr<Horse::Scene> GetActiveScene() const { return m_ActiveScene; }
  Horse::Entity GetSelectedEntity() const { return m_SelectedEntity; }
  void SetSelectedEntity(Horse::Entity entity);

private slots:
  void OnNewScene();
  void OnOpenScene();
  void OnSaveScene();
  void OnSaveSceneAs();
  void OnExit();

private:
  void CreateMenus();
  void CreatePanels();
  void CreateToolBar();
  void NewScene();
  void OpenScene(const std::string &filepath);
  void SaveScene(const std::string &filepath);

  D3D11ViewportWidget *m_ViewportWidget = nullptr;
  HierarchyPanel *m_HierarchyPanel = nullptr;
  InspectorPanel *m_InspectorPanel = nullptr;
  ContentBrowserPanel *m_ContentBrowserPanel = nullptr;
  ConsolePanel *m_ConsolePanel = nullptr;

  std::shared_ptr<Horse::Scene> m_ActiveScene;
  Horse::Entity m_SelectedEntity;
  std::string m_CurrentScenePath;
};

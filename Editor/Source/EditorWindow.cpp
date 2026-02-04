#include "EditorWindow.h"
#include "D3D11ViewportWidget.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"

#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>

EditorWindow::EditorWindow(QWidget *parent) : QMainWindow(parent) {

  setWindowTitle("Horse Engine Editor");
  resize(1600, 900);

  CreateMenus();
  CreatePanels();
  CreateToolBar();

  // Create default scene
  NewScene();
}

EditorWindow::~EditorWindow() {}

void EditorWindow::SetSelectedEntity(Horse::Entity entity) {
  m_SelectedEntity = entity;

  // Update inspector panel
  if (m_InspectorPanel) {
    m_InspectorPanel->SetSelectedEntity(entity);
  }
}

void EditorWindow::CreateMenus() {
  QMenu *fileMenu = menuBar()->addMenu("&File");

  QAction *newSceneAction = fileMenu->addAction("&New Scene");
  newSceneAction->setShortcut(QKeySequence::New);
  connect(newSceneAction, &QAction::triggered, this, &EditorWindow::OnNewScene);

  QAction *openSceneAction = fileMenu->addAction("&Open Scene...");
  openSceneAction->setShortcut(QKeySequence::Open);
  connect(openSceneAction, &QAction::triggered, this,
          &EditorWindow::OnOpenScene);

  QAction *saveSceneAction = fileMenu->addAction("&Save Scene");
  saveSceneAction->setShortcut(QKeySequence::Save);
  connect(saveSceneAction, &QAction::triggered, this,
          &EditorWindow::OnSaveScene);

  QAction *saveSceneAsAction = fileMenu->addAction("Save Scene &As...");
  saveSceneAsAction->setShortcut(QKeySequence::SaveAs);
  connect(saveSceneAsAction, &QAction::triggered, this,
          &EditorWindow::OnSaveSceneAs);

  fileMenu->addSeparator();

  QAction *exitAction = fileMenu->addAction("E&xit");
  connect(exitAction, &QAction::triggered, this, &EditorWindow::OnExit);

  QMenu *editMenu = menuBar()->addMenu("&Edit");
  editMenu->addAction("&Preferences...");

  QMenu *viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction("Reset Layout");

  QMenu *helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction("&About");
}

void EditorWindow::CreatePanels() {
  m_ViewportWidget = new D3D11ViewportWidget(this);
  setCentralWidget(m_ViewportWidget);

  QDockWidget *hierarchyDock = new QDockWidget("Hierarchy", this);
  m_HierarchyPanel = new HierarchyPanel(hierarchyDock);
  hierarchyDock->setWidget(m_HierarchyPanel);
  addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

  // Connect hierarchy selection signal
  connect(m_HierarchyPanel, &HierarchyPanel::EntitySelected, this,
          &EditorWindow::SetSelectedEntity);

  QDockWidget *inspectorDock = new QDockWidget("Inspector", this);
  m_InspectorPanel = new InspectorPanel(inspectorDock);
  inspectorDock->setWidget(m_InspectorPanel);
  addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

  QDockWidget *contentBrowserDock = new QDockWidget("Content Browser", this);
  m_ContentBrowserPanel = new ContentBrowserPanel(contentBrowserDock);
  contentBrowserDock->setWidget(m_ContentBrowserPanel);
  addDockWidget(Qt::BottomDockWidgetArea, contentBrowserDock);

  QDockWidget *consoleDock = new QDockWidget("Console", this);
  m_ConsolePanel = new ConsolePanel(consoleDock);
  consoleDock->setWidget(m_ConsolePanel);
  addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

  tabifyDockWidget(contentBrowserDock, consoleDock);
}

void EditorWindow::CreateToolBar() {
  QToolBar *toolbar = addToolBar("Main Toolbar");
  toolbar->addAction("Play");
  toolbar->addAction("Pause");
  toolbar->addAction("Stop");
}

void EditorWindow::NewScene() {
  m_ActiveScene = std::make_shared<Horse::Scene>("Untitled Scene");
  m_CurrentScenePath.clear();
  m_SelectedEntity = {};

  // Update panels
  if (m_HierarchyPanel) {
    m_HierarchyPanel->SetScene(m_ActiveScene);
  }
  if (m_InspectorPanel) {
    m_InspectorPanel->SetSelectedEntity({});
  }

  setWindowTitle("Horse Engine Editor - Untitled Scene*");
}

#include "HorseEngine/Scene/SceneSerializer.h"

void EditorWindow::OpenScene(const std::string &filepath) {
  auto scene = Horse::SceneSerializer::DeserializeFromJSON(filepath);
  if (scene) {
    m_ActiveScene = scene;
    m_CurrentScenePath = filepath;
    m_SelectedEntity = {};

    if (m_HierarchyPanel) {
      m_HierarchyPanel->SetScene(m_ActiveScene);
    }
    if (m_InspectorPanel) {
      m_InspectorPanel->SetSelectedEntity({});
    }

    setWindowTitle(QString("Horse Engine Editor - %1")
                       .arg(QString::fromStdString(filepath)));
  } else {
    QMessageBox::critical(this, "Error", "Failed to load scene!");
  }
}

void EditorWindow::SaveScene(const std::string &filepath) {
  if (Horse::SceneSerializer::SerializeToJSON(m_ActiveScene.get(), filepath)) {
    m_CurrentScenePath = filepath;
    setWindowTitle(QString("Horse Engine Editor - %1")
                       .arg(QString::fromStdString(filepath)));
  } else {
    QMessageBox::critical(this, "Error", "Failed to save scene!");
  }
}

void EditorWindow::OnNewScene() {
  // TODO: Prompt to save current scene if modified
  NewScene();
}

void EditorWindow::OnOpenScene() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open Scene", "",
      "Horse Level Files (*.horselevel.json);;All Files (*)");

  if (!filename.isEmpty()) {
    OpenScene(filename.toStdString());
  }
}

void EditorWindow::OnSaveScene() {
  if (m_CurrentScenePath.empty()) {
    OnSaveSceneAs();
  } else {
    SaveScene(m_CurrentScenePath);
  }
}

void EditorWindow::OnSaveSceneAs() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save Scene As", "",
      "Horse Level Files (*.horselevel.json);;All Files (*)");

  if (!filename.isEmpty()) {
    SaveScene(filename.toStdString());
  }
}

void EditorWindow::OnExit() { close(); }

#include "EditorWindow.h"
#include "D3D11ViewportWidget.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/GameViewport.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/SceneViewport.h"

#include "Dialogs/PreferencesDialog.h"
#include "EditorPreferences.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Time.h"
#include "HorseEngine/Render/MaterialRegistry.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"
#include "ThemeManager.h"

#include <QCoreApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QToolBar>

EditorWindow::EditorWindow(std::shared_ptr<void> logSink, QWidget *parent)
    : QMainWindow(parent), m_LogSink(logSink) {

  setWindowTitle("Horse Engine Editor");
  resize(1600, 900);

  CreateMenus();
  CreatePanels();
  CreateToolBar();

  // Apply saved theme
  Horse::ThemeManager::ApplyTheme(
      Horse::EditorPreferences::Get().GetAppearance());

  // Create default scene
  NewScene();

  // Load layout if it exists
  // Load layout if it exists
  OnLoadLayout();

  // Create update timer (60 FPS)
  m_UpdateTimer = new QTimer(this);
  connect(m_UpdateTimer, &QTimer::timeout, this, &EditorWindow::OnUpdate);
  m_UpdateTimer->start(16);
}

EditorWindow::~EditorWindow() {}

void EditorWindow::SetSelectedEntity(Horse::Entity entity) {
  m_SelectedEntity = entity;

  // Update inspector panel
  if (m_InspectorPanel) {
    m_InspectorPanel->SetSelectedEntity(entity);
  }
}

void EditorWindow::UpdateSceneContext() {
  if (m_HierarchyPanel) {
    m_HierarchyPanel->SetScene(m_ActiveScene);
  }
  if (m_SceneViewport) {
    m_SceneViewport->SetScene(m_ActiveScene);
  }
  if (m_GameViewport) {
    m_GameViewport->SetScene(m_ActiveScene);
  }
  if (m_InspectorPanel) {
    m_InspectorPanel->SetSelectedEntity({});
  }
}

void EditorWindow::OnUpdate() {
  Horse::Time::Update();

  if (m_ActiveScene) {
    m_ActiveScene->OnUpdate(Horse::Time::GetDeltaTime());
  }

  if (m_SceneViewport)
    m_SceneViewport->update();
  if (m_GameViewport)
    m_GameViewport->update();
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

  QAction *newProjectAction = fileMenu->addAction("New &Project...");
  connect(newProjectAction, &QAction::triggered, this,
          &EditorWindow::OnNewProject);

  QAction *openProjectAction = fileMenu->addAction("&Open Project...");
  connect(openProjectAction, &QAction::triggered, this,
          &EditorWindow::OnOpenProject);

  QAction *saveProjectAction = fileMenu->addAction("Sa&ve Project");
  connect(saveProjectAction, &QAction::triggered, this,
          &EditorWindow::OnSaveProject);

  fileMenu->addSeparator();

  QAction *exitAction = fileMenu->addAction("E&xit");
  connect(exitAction, &QAction::triggered, this, &EditorWindow::OnExit);

  QMenu *editMenu = menuBar()->addMenu("&Edit");
  QAction *projectSettingsAction = editMenu->addAction("&Project Settings...");
  connect(projectSettingsAction, &QAction::triggered, this,
          &EditorWindow::OnProjectSettings);

  QAction *preferencesAction = editMenu->addAction("&Preferences...");
  connect(preferencesAction, &QAction::triggered, this,
          &EditorWindow::OnPreferences);

  QMenu *viewMenu = menuBar()->addMenu("&View");

  QAction *saveLayoutAction = viewMenu->addAction("Save Layout");
  connect(saveLayoutAction, &QAction::triggered, this,
          &EditorWindow::OnSaveLayout);

  QAction *loadLayoutAction = viewMenu->addAction("Load Layout");
  connect(loadLayoutAction, &QAction::triggered, this,
          &EditorWindow::OnLoadLayout);

  viewMenu->addSeparator();

  QAction *resetLayoutAction = viewMenu->addAction("Reset Layout");
  connect(resetLayoutAction, &QAction::triggered, this,
          &EditorWindow::OnResetLayout);

  QMenu *helpMenu = menuBar()->addMenu("&Help");
  helpMenu->addAction("&About");
}

void EditorWindow::CreatePanels() {
  setCentralWidget(nullptr);

  QDockWidget *sceneDock = new QDockWidget("Scene", this);
  m_SceneViewport = new SceneViewport(sceneDock);
  sceneDock->setWidget(m_SceneViewport);
  addDockWidget(Qt::TopDockWidgetArea, sceneDock);

  QDockWidget *gameDock = new QDockWidget("Game", this);
  m_GameViewport = new GameViewport(gameDock);
  gameDock->setWidget(m_GameViewport);
  addDockWidget(Qt::TopDockWidgetArea, gameDock);

  tabifyDockWidget(sceneDock, gameDock);
  sceneDock->raise(); // Scene by default

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
  if (m_LogSink)
    m_ConsolePanel->SetSink(m_LogSink);
  consoleDock->setWidget(m_ConsolePanel);
  addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

  tabifyDockWidget(contentBrowserDock, consoleDock);
}

void EditorWindow::CreateToolBar() {
  QToolBar *toolbar = addToolBar("Main Toolbar");

  QAction *playAction = toolbar->addAction("Play");
  connect(playAction, &QAction::triggered, this, &EditorWindow::OnPlay);

  QAction *pauseAction = toolbar->addAction("Pause");
  connect(pauseAction, &QAction::triggered, this, &EditorWindow::OnPause);

  QAction *stopAction = toolbar->addAction("Stop");
  connect(stopAction, &QAction::triggered, this, &EditorWindow::OnStop);
}

void EditorWindow::NewScene() {
  m_EditorScene = std::make_shared<Horse::Scene>("Untitled Scene");
  m_ActiveScene = m_EditorScene;
  m_CurrentScenePath.clear();
  m_SelectedEntity = {};

  // Create default camera
  auto cameraEntity = m_ActiveScene->CreateEntity("Main Camera");
  auto &camera = cameraEntity.AddComponent<Horse::CameraComponent>();
  camera.Primary = true;
  auto &transform = cameraEntity.GetComponent<Horse::TransformComponent>();
  transform.Position = {0.0f, 2.0f, -5.0f};

  UpdateSceneContext();

  setWindowTitle("Horse Engine Editor - Untitled Scene*");
}

#include "HorseEngine/Scene/SceneSerializer.h"

void EditorWindow::OpenScene(const std::string &filepath) {
  auto scene = Horse::SceneSerializer::DeserializeFromJSON(filepath);
  if (scene) {
    m_EditorScene = scene;
    m_ActiveScene = m_EditorScene;
    m_CurrentScenePath = filepath;
    m_SelectedEntity = {};

    UpdateSceneContext();

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

#include "HorseEngine/Project/Project.h"
#include "HorseEngine/Project/ProjectSerializer.h"

void EditorWindow::OnNewProject() {
  QString filename = QFileDialog::getSaveFileName(
      this, "New Project", "",
      "Horse Project Files (*.horseproject.json);;All Files (*)");

  if (!filename.isEmpty()) {
    NewProject(filename.toStdString());
  }
}

void EditorWindow::OnOpenProject() {
  QString filename = QFileDialog::getOpenFileName(
      this, "Open Project", "",
      "Horse Project Files (*.horseproject.json);;All Files (*)");

  if (!filename.isEmpty()) {
    OpenProject(filename.toStdString());
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

void EditorWindow::OnSaveProject() {
  auto project = Horse::Project::GetActive();
  if (project) {
    std::filesystem::path projectPath = project->GetConfig().ProjectDirectory /
                                        project->GetConfig().ProjectFileName;
    SaveProject(projectPath.string());
  }
}

void EditorWindow::NewProject(const std::string &filepath) {
  auto project = std::make_shared<Horse::Project>();
  std::filesystem::path p(filepath);
  std::string stem = p.stem().string();

  // Strip .horseproject if it's part of the stem
  size_t pos = stem.find(".horseproject");
  if (pos != std::string::npos) {
    stem = stem.substr(0, pos);
  }

  project->GetConfig().Name = stem;
  project->GetConfig().GUID = Horse::UUID().ToString();
  project->GetConfig().ProjectFileName = p.filename();
  project->GetConfig().ProjectDirectory = p.parent_path();

  // Create asset directory if it doesn't exist
  std::filesystem::create_directories(project->GetConfig().ProjectDirectory /
                                      project->GetConfig().AssetDirectory);
  std::filesystem::create_directories(project->GetConfig().ProjectDirectory /
                                      project->GetConfig().AssetDirectory /
                                      "Scenes");
  std::filesystem::create_directories(project->GetConfig().ProjectDirectory /
                                      project->GetConfig().AssetDirectory /
                                      "Materials");
  std::filesystem::create_directories(project->GetConfig().ProjectDirectory /
                                      project->GetConfig().AssetDirectory /
                                      "Scripts");

  Horse::Project::SetActive(project);
  Horse::Project::SetActive(project);
  SaveProject(filepath);

  // Initialize Asset Manager
  Horse::AssetManager::Get().Initialize(project->GetConfig().ProjectDirectory /
                                        project->GetConfig().AssetDirectory);

  if (m_ContentBrowserPanel) {
    m_ContentBrowserPanel->Refresh();
  }

  NewScene(); // Start with a clean scene in the new project
}

void EditorWindow::OpenProject(const std::string &filepath) {
  auto project = std::make_shared<Horse::Project>();
  Horse::ProjectSerializer serializer(project);
  if (serializer.DeserializeFromJSON(filepath)) {
    Horse::Project::SetActive(project);

    // Initialize Asset Manager
    Horse::AssetManager::Get().Initialize(
        project->GetConfig().ProjectDirectory /
        project->GetConfig().AssetDirectory);

    // Load materials
    std::filesystem::path materialPath = project->GetConfig().ProjectDirectory /
                                         project->GetConfig().AssetDirectory /
                                         "Materials";

    Horse::MaterialRegistry::Get().LoadMaterialsFromDirectory(
        materialPath.string());

    // Load default scene if available
    if (!project->GetConfig().DefaultScene.empty()) {
      std::filesystem::path scenePath = project->GetConfig().ProjectDirectory /
                                        project->GetConfig().DefaultScene;
      if (std::filesystem::exists(scenePath)) {
        OpenScene(scenePath.string());
      } else {
        NewScene();
      }
    } else {
      NewScene();
    }

    setWindowTitle(QString("Horse Engine Editor - %1")
                       .arg(QString::fromStdString(project->GetConfig().Name)));

    if (m_ContentBrowserPanel) {
      m_ContentBrowserPanel->Refresh();
    }
  } else {
    QMessageBox::critical(this, "Error", "Failed to load project!");
  }
}

void EditorWindow::SaveProject(const std::string &filepath) {
  auto project = Horse::Project::GetActive();
  if (!project)
    return;

  Horse::ProjectSerializer serializer(project);
  if (!serializer.SerializeToJSON(filepath)) {
    QMessageBox::critical(this, "Error", "Failed to save project!");
  }
}

#include "Dialogs/ProjectSettingsDialog.h"

void EditorWindow::OnProjectSettings() {
  auto project = Horse::Project::GetActive();
  if (!project) {
    QMessageBox::warning(this, "Warning", "No active project found!");
    return;
  }

  ProjectSettingsDialog dialog(project, this);
  if (dialog.exec() == QDialog::Accepted) {
    auto &config = project->GetConfig();
    std::string oldName = config.Name;
    std::string newName = dialog.GetProjectName();

    config.Name = newName;
    config.EngineVersion = dialog.GetEngineVersion();
    config.DefaultScene = dialog.GetDefaultScene();

    // Handle project file renaming if name changed
    if (oldName != newName) {
      std::filesystem::path oldPath =
          config.ProjectDirectory / config.ProjectFileName;

      std::string newFileNameStr = newName;
      if (newFileNameStr.find(".horseproject") == std::string::npos) {
        newFileNameStr += ".horseproject.json";
      } else if (!newFileNameStr.ends_with(".json")) {
        newFileNameStr += ".json";
      }

      std::filesystem::path newFileName(newFileNameStr);
      std::filesystem::path newPath = config.ProjectDirectory / newFileName;

      try {
        if (std::filesystem::exists(oldPath)) {
          std::filesystem::rename(oldPath, newPath);
          config.ProjectFileName = newFileName;
        }
      } catch (const std::exception &e) {
        QMessageBox::critical(
            this, "Error",
            QString("Failed to rename project file: %1").arg(e.what()));
      }
    }

    SaveProject((config.ProjectDirectory / config.ProjectFileName).string());
    setWindowTitle(QString("Horse Engine Editor - %1")
                       .arg(QString::fromStdString(config.Name)));
  }
}

void EditorWindow::OnExit() { close(); }

void EditorWindow::OnPreferences() {
  Horse::PreferencesDialog dialog(this);
  dialog.exec();
}

void EditorWindow::OnSaveLayout() {
  QSettings settings(QCoreApplication::applicationDirPath() + "/layout.ini",
                     QSettings::IniFormat);
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
}

void EditorWindow::OnLoadLayout() {
  QSettings settings(QCoreApplication::applicationDirPath() + "/layout.ini",
                     QSettings::IniFormat);
  if (settings.contains("geometry")) {
    restoreGeometry(settings.value("geometry").toByteArray());
  }
  if (settings.contains("windowState")) {
    restoreState(settings.value("windowState").toByteArray());
  }
}

void EditorWindow::OnResetLayout() {
  // Simple way to reset: remove existing docks and recreate them
  // But a better way is to define a "default" state
  // For now, let's just use a hardcoded layout or similar
  // Actually, just calling CreatePanels again might duplicate things.
  // The user might just want a "factory reset".

  // To keep it simple and safe for now:
  QMessageBox::information(this, "Reset Layout",
                           "Restart the editor with the layout.ini deleted to "
                           "fully reset to default.");
}

void EditorWindow::OnPlay() {
  if (m_ActiveScene && m_ActiveScene->GetState() == Horse::SceneState::Edit) {
    m_EditorScene = m_ActiveScene;
    m_RuntimeScene = Horse::Scene::Copy(m_EditorScene);
    m_ActiveScene = m_RuntimeScene;
    UpdateSceneContext();
    m_ActiveScene->OnRuntimeStart();
  }
}

void EditorWindow::OnPause() {
  if (m_ActiveScene) {
    if (m_ActiveScene->GetState() == Horse::SceneState::Play) {
      m_ActiveScene->SetState(Horse::SceneState::Pause);
    } else if (m_ActiveScene->GetState() == Horse::SceneState::Pause) {
      m_ActiveScene->SetState(Horse::SceneState::Play);
    }
  }
}

void EditorWindow::OnStop() {
  if (m_ActiveScene && m_ActiveScene->GetState() != Horse::SceneState::Edit) {
    m_ActiveScene->OnRuntimeStop();
    m_ActiveScene = m_EditorScene;
    m_RuntimeScene.reset();
    UpdateSceneContext();
  }
}

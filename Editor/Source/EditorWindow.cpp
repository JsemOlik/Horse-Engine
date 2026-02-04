#include "EditorWindow.h"
#include "D3D11ViewportWidget.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"

#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QMessageBox>

EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent) {
    
    setWindowTitle("Horse Engine Editor");
    resize(1600, 900);
    
    CreateMenus();
    CreatePanels();
    CreateToolBar();
}

EditorWindow::~EditorWindow() {
}

void EditorWindow::CreateMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* newSceneAction = fileMenu->addAction("&New Scene");
    connect(newSceneAction, &QAction::triggered, this, &EditorWindow::OnNewScene);
    
    QAction* openSceneAction = fileMenu->addAction("&Open Scene...");
    connect(openSceneAction, &QAction::triggered, this, &EditorWindow::OnOpenScene);
    
    QAction* saveSceneAction = fileMenu->addAction("&Save Scene");
    connect(saveSceneAction, &QAction::triggered, this, &EditorWindow::OnSaveScene);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &EditorWindow::OnExit);
    
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("&Preferences...");
    
    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Reset Layout");
    
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About");
}

void EditorWindow::CreatePanels() {
    m_ViewportWidget = new D3D11ViewportWidget(this);
    setCentralWidget(m_ViewportWidget);
    
    QDockWidget* hierarchyDock = new QDockWidget("Hierarchy", this);
    m_HierarchyPanel = new HierarchyPanel(hierarchyDock);
    hierarchyDock->setWidget(m_HierarchyPanel);
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);
    
    QDockWidget* inspectorDock = new QDockWidget("Inspector", this);
    m_InspectorPanel = new InspectorPanel(inspectorDock);
    inspectorDock->setWidget(m_InspectorPanel);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    
    QDockWidget* contentBrowserDock = new QDockWidget("Content Browser", this);
    m_ContentBrowserPanel = new ContentBrowserPanel(contentBrowserDock);
    contentBrowserDock->setWidget(m_ContentBrowserPanel);
    addDockWidget(Qt::BottomDockWidgetArea, contentBrowserDock);
    
    QDockWidget* consoleDock = new QDockWidget("Console", this);
    m_ConsolePanel = new ConsolePanel(consoleDock);
    consoleDock->setWidget(m_ConsolePanel);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    
    tabifyDockWidget(contentBrowserDock, consoleDock);
}

void EditorWindow::CreateToolBar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction("Play");
    toolbar->addAction("Pause");
    toolbar->addAction("Stop");
}

void EditorWindow::OnNewScene() {
}

void EditorWindow::OnOpenScene() {
}

void EditorWindow::OnSaveScene() {
}

void EditorWindow::OnExit() {
    close();
}

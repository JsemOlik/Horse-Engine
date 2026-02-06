#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <filesystem>

namespace Horse {
class Project;
}

enum class BuildToolType { Cook, Package };

class BuildDialog : public QDialog {
  Q_OBJECT

public:
  BuildDialog(BuildToolType type, QWidget *parent = nullptr);
  ~BuildDialog();

private slots:
  void OnBrowseSource();
  void OnBrowseOutput();
  void OnBrowseTool();
  void OnBrowseRunner();
  void OnBrowseGameDLL();
  void OnRunBuild();
  void OnOpenFolder();
  void OnProcessOutput();
  void OnProcessFinished(int exitCode);

private:
  void SetupUI();
  void LoadSettings();
  void SaveSettings();
  void UpdateControls(bool running);

  BuildToolType m_Type;

  QLineEdit *m_SourceEdit;
  QLineEdit *m_OutputEdit;
  QLineEdit *m_ToolEdit;

  // For Packaging specifically
  QLineEdit *m_RunnerEdit = nullptr;
  QLineEdit *m_GameDLLEdit = nullptr;
  QLineEdit *m_GameNameEdit = nullptr;

  QTextEdit *m_LogArea;
  QPushButton *m_BuildButton;
  QPushButton *m_OpenFolderButton;

  QProcess *m_Process;
  QString m_LastOutputPath;
};

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <filesystem>
#include <string>
#include <vector>

namespace Horse {
class Project;
}

class ProjectSettingsDialog : public QDialog {
  Q_OBJECT

public:
  ProjectSettingsDialog(std::shared_ptr<Horse::Project> project,
                        QWidget *parent = nullptr);
  ~ProjectSettingsDialog() = default;

  std::string GetProjectName() const {
    return m_NameEdit->text().toStdString();
  }
  std::string GetEngineVersion() const {
    return m_VersionEdit->text().toStdString();
  }
  std::string GetDefaultScene() const {
    return m_SceneCombo->currentText().toStdString();
  }
  std::string GetSkyboxTexture() const {
    return m_SkyboxCombo->currentText().toStdString();
  }

private:
  void ScanForScenes();
  void ScanForTextures();

  std::shared_ptr<Horse::Project> m_Project;
  QLineEdit *m_NameEdit;
  QLineEdit *m_VersionEdit;
  QComboBox *m_SceneCombo;
  QComboBox *m_SkyboxCombo;
};

#include "ProjectSettingsDialog.h"
#include "HorseEngine/Project/Project.h"

#include <QMessageBox>

ProjectSettingsDialog::ProjectSettingsDialog(
    std::shared_ptr<Horse::Project> project, QWidget *parent)
    : QDialog(parent), m_Project(project) {

  setWindowTitle("Project Settings");
  setMinimumWidth(400);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  QFormLayout *formLayout = new QFormLayout();

  const auto &config = m_Project->GetConfig();

  m_NameEdit = new QLineEdit(QString::fromStdString(config.Name));
  m_VersionEdit = new QLineEdit(QString::fromStdString(config.EngineVersion));
  m_SceneCombo = new QComboBox();

  ScanForScenes();

  // Select current default scene if it exists in the list
  int index =
      m_SceneCombo->findText(QString::fromStdString(config.DefaultScene));
  if (index != -1) {
    m_SceneCombo->setCurrentIndex(index);
  }

  formLayout->addRow("Project Name:", m_NameEdit);
  formLayout->addRow("Engine Version:", m_VersionEdit);
  formLayout->addRow("Default Scene:", m_SceneCombo);

  mainLayout->addLayout(formLayout);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  QPushButton *okButton = new QPushButton("Save");
  QPushButton *cancelButton = new QPushButton("Cancel");

  connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  buttonLayout->addStretch();
  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);

  mainLayout->addLayout(buttonLayout);
}

void ProjectSettingsDialog::ScanForScenes() {
  m_SceneCombo->clear();
  m_SceneCombo->addItem(""); // Empty option for no default scene

  if (!m_Project)
    return;

  std::filesystem::path projectDir = m_Project->GetConfig().ProjectDirectory;
  if (!std::filesystem::exists(projectDir))
    return;

  try {
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(projectDir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        std::string filename = entry.path().filename().string();
        if (filename.find(".horselevel.json") != std::string::npos) {
          // Store path relative to project directory
          std::string relativePath =
              std::filesystem::relative(entry.path(), projectDir)
                  .generic_string();
          m_SceneCombo->addItem(QString::fromStdString(relativePath));
        }
      }
    }
  } catch (const std::exception &e) {
    // Log error silently or show message
  }
}

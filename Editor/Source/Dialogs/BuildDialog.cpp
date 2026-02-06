#include "BuildDialog.h"
#include "HorseEngine/Project/Project.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

BuildDialog::BuildDialog(BuildToolType type, QWidget *parent)
    : QDialog(parent), m_Type(type), m_Process(new QProcess(this)) {

  setWindowTitle(m_Type == BuildToolType::Cook ? "Cook Project"
                                               : "Package Project");
  setMinimumWidth(600);
  setMinimumHeight(400);

  SetupUI();
  LoadSettings();

  connect(m_Process, &QProcess::readyReadStandardOutput, this,
          &BuildDialog::OnProcessOutput);
  connect(m_Process, &QProcess::readyReadStandardError, this,
          &BuildDialog::OnProcessOutput);
  connect(m_Process, SIGNAL(finished(int)), this, SLOT(OnProcessFinished(int)));
}

BuildDialog::~BuildDialog() { SaveSettings(); }

void BuildDialog::SetupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  QFormLayout *formLayout = new QFormLayout();

  // Source Path
  QHBoxLayout *sourceLayout = new QHBoxLayout();
  m_SourceEdit = new QLineEdit();
  QPushButton *browseSourceBtn = new QPushButton("Browse...");
  connect(browseSourceBtn, &QPushButton::clicked, this,
          &BuildDialog::OnBrowseSource);
  sourceLayout->addWidget(m_SourceEdit);
  sourceLayout->addWidget(browseSourceBtn);
  formLayout->addRow(m_Type == BuildToolType::Cook ? "Asset Directory:"
                                                   : "Cooked Output:",
                     sourceLayout);

  // Output Path
  QHBoxLayout *outputLayout = new QHBoxLayout();
  m_OutputEdit = new QLineEdit();
  QPushButton *browseOutputBtn = new QPushButton("Browse...");
  connect(browseOutputBtn, &QPushButton::clicked, this,
          &BuildDialog::OnBrowseOutput);
  outputLayout->addWidget(m_OutputEdit);
  outputLayout->addWidget(browseOutputBtn);
  formLayout->addRow("Output Directory:", outputLayout);

  // Tool Path
  QHBoxLayout *toolLayout = new QHBoxLayout();
  m_ToolEdit = new QLineEdit();
  QPushButton *browseToolBtn = new QPushButton("Browse...");
  connect(browseToolBtn, &QPushButton::clicked, this,
          &BuildDialog::OnBrowseTool);
  toolLayout->addWidget(m_ToolEdit);
  toolLayout->addWidget(browseToolBtn);
  formLayout->addRow("Tool Path (Exe):", toolLayout);

  if (m_Type == BuildToolType::Package) {
    // Runner Exe
    QHBoxLayout *runnerLayout = new QHBoxLayout();
    m_RunnerEdit = new QLineEdit();
    QPushButton *browseRunnerBtn = new QPushButton("Browse...");
    connect(browseRunnerBtn, &QPushButton::clicked, this,
            &BuildDialog::OnBrowseRunner);
    runnerLayout->addWidget(m_RunnerEdit);
    runnerLayout->addWidget(browseRunnerBtn);
    formLayout->addRow("Runner Path (Exe):", runnerLayout);

    // Game DLL
    QHBoxLayout *gameDLLLayout = new QHBoxLayout();
    m_GameDLLEdit = new QLineEdit();
    QPushButton *browseGameDLLBtn = new QPushButton("Browse...");
    connect(browseGameDLLBtn, &QPushButton::clicked, this,
            &BuildDialog::OnBrowseGameDLL);
    gameDLLLayout->addWidget(m_GameDLLEdit);
    gameDLLLayout->addWidget(browseGameDLLBtn);
    formLayout->addRow("Game DLL Path:", gameDLLLayout);

    // Game Name
    m_GameNameEdit = new QLineEdit();
    formLayout->addRow("Game Name:", m_GameNameEdit);
  }

  mainLayout->addLayout(formLayout);

  m_LogArea = new QTextEdit();
  m_LogArea->setReadOnly(true);
  m_LogArea->setFontFamily("Courier New");
  mainLayout->addWidget(m_LogArea);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  m_BuildButton = new QPushButton("Run Build");
  m_BuildButton->setMinimumHeight(35);
  m_BuildButton->setStyleSheet(
      "background-color: #2a82da; color: white; font-weight: bold;");
  connect(m_BuildButton, &QPushButton::clicked, this, &BuildDialog::OnRunBuild);

  m_OpenFolderButton = new QPushButton("Open Output Folder");
  m_OpenFolderButton->setEnabled(false);
  connect(m_OpenFolderButton, &QPushButton::clicked, this,
          &BuildDialog::OnOpenFolder);

  buttonLayout->addWidget(m_BuildButton);
  buttonLayout->addWidget(m_OpenFolderButton);
  mainLayout->addLayout(buttonLayout);
}

void BuildDialog::LoadSettings() {
  QSettings settings("HorseEngine", "Editor");
  QString group = m_Type == BuildToolType::Cook ? "CookTool" : "PackageTool";
  settings.beginGroup(group);

  m_SourceEdit->setText(settings.value("SourcePath").toString());
  m_OutputEdit->setText(settings.value("OutputPath").toString());
  m_ToolEdit->setText(settings.value("ToolPath").toString());

  if (m_Type == BuildToolType::Package) {
    m_RunnerEdit->setText(settings.value("RunnerPath").toString());
    m_GameDLLEdit->setText(settings.value("GameDLLPath").toString());
    m_GameNameEdit->setText(settings.value("GameName", "MyGame").toString());
  }

  // Default heuristics if empty
  if (m_ToolEdit->text().isEmpty()) {
    QString appDir = QCoreApplication::applicationDirPath();
    QString toolName =
        m_Type == BuildToolType::Cook ? "HorseCooker.exe" : "HorsePackager.exe";
    // Look in Tools/Debug relative to app dir
    QFileInfo toolInfo(appDir + "/../../Tools/Debug/" + toolName);
    if (toolInfo.exists()) {
      m_ToolEdit->setText(toolInfo.absoluteFilePath());
    }
  }

  if (m_Type == BuildToolType::Package) {
    if (m_RunnerEdit->text().isEmpty()) {
      QFileInfo runnerInfo(QCoreApplication::applicationDirPath() +
                           "/HorseRunner.exe");
      if (runnerInfo.exists())
        m_RunnerEdit->setText(runnerInfo.absoluteFilePath());
    }
    if (m_GameDLLEdit->text().isEmpty()) {
      QFileInfo dllInfo(QCoreApplication::applicationDirPath() +
                        "/HorseGame.dll");
      if (dllInfo.exists())
        m_GameDLLEdit->setText(dllInfo.absoluteFilePath());
    }
  }

  settings.endGroup();
}

void BuildDialog::SaveSettings() {
  QSettings settings("HorseEngine", "Editor");
  QString group = m_Type == BuildToolType::Cook ? "CookTool" : "PackageTool";
  settings.beginGroup(group);

  settings.setValue("SourcePath", m_SourceEdit->text());
  settings.setValue("OutputPath", m_OutputEdit->text());
  settings.setValue("ToolPath", m_ToolEdit->text());

  if (m_Type == BuildToolType::Package) {
    settings.setValue("RunnerPath", m_RunnerEdit->text());
    settings.setValue("GameDLLPath", m_GameDLLEdit->text());
    settings.setValue("GameName", m_GameNameEdit->text());
  }

  settings.endGroup();
}

void BuildDialog::OnBrowseSource() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Source Directory", m_SourceEdit->text());
  if (!dir.isEmpty())
    m_SourceEdit->setText(dir);
}

void BuildDialog::OnBrowseOutput() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory", m_OutputEdit->text());
  if (!dir.isEmpty())
    m_OutputEdit->setText(dir);
}

void BuildDialog::OnBrowseTool() {
  QString file = QFileDialog::getOpenFileName(
      this, "Select Build Tool", m_ToolEdit->text(),
      "Executables (*.exe);;All Files (*)");
  if (!file.isEmpty())
    m_ToolEdit->setText(file);
}

void BuildDialog::OnBrowseRunner() {
  QString file = QFileDialog::getOpenFileName(
      this, "Select Runner Executable", m_RunnerEdit->text(),
      "Executables (*.exe);;All Files (*)");
  if (!file.isEmpty())
    m_RunnerEdit->setText(file);
}

void BuildDialog::OnBrowseGameDLL() {
  QString file = QFileDialog::getOpenFileName(
      this, "Select Game DLL", m_GameDLLEdit->text(),
      "DLL Files (*.dll);;All Files (*)");
  if (!file.isEmpty())
    m_GameDLLEdit->setText(file);
}

void BuildDialog::OnRunBuild() {
  m_LogArea->clear();
  m_LogArea->append("<b>Preparing build...</b>");

  QString tool = m_ToolEdit->text();
  QStringList args;

  if (m_Type == BuildToolType::Cook) {
    args << m_SourceEdit->text() << m_OutputEdit->text();
  } else {
    args << m_SourceEdit->text() << m_OutputEdit->text() << m_RunnerEdit->text()
         << m_GameDLLEdit->text() << m_GameNameEdit->text();
  }

  m_LastOutputPath = m_OutputEdit->text();
  m_Process->start(tool, args);

  if (!m_Process->waitForStarted()) {
    m_LogArea->append(
        "<font color='red'><b>Failed to start build tool!</b></font>");
    return;
  }

  UpdateControls(true);
}

void BuildDialog::OnOpenFolder() {
  if (!m_LastOutputPath.isEmpty()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_LastOutputPath));
  }
}

void BuildDialog::OnProcessOutput() {
  QByteArray out = m_Process->readAllStandardOutput();
  QByteArray err = m_Process->readAllStandardError();
  if (!out.isEmpty())
    m_LogArea->append(QString::fromLocal8Bit(out));
  if (!err.isEmpty())
    m_LogArea->append("<font color='orange'>" + QString::fromLocal8Bit(err) +
                      "</font>");
}

void BuildDialog::OnProcessFinished(int exitCode) {
  UpdateControls(false);
  if (exitCode == 0) {
    m_LogArea->append(
        "<br><font color='green'><b>Build Completed Successfully!</b></font>");
    m_OpenFolderButton->setEnabled(true);
  } else {
    m_LogArea->append(QString("<br><font color='red'><b>Build Failed with exit "
                              "code %1</b></font>")
                          .arg(exitCode));
  }
}

void BuildDialog::UpdateControls(bool running) {
  m_BuildButton->setEnabled(!running);
  m_SourceEdit->setEnabled(!running);
  m_OutputEdit->setEnabled(!running);
  m_ToolEdit->setEnabled(!running);
  if (m_RunnerEdit)
    m_RunnerEdit->setEnabled(!running);
  if (m_GameDLLEdit)
    m_GameDLLEdit->setEnabled(!running);
  if (m_GameNameEdit)
    m_GameNameEdit->setEnabled(!running);

  if (running) {
    m_BuildButton->setText("Building...");
    m_OpenFolderButton->setEnabled(false);
  } else {
    m_BuildButton->setText("Run Build");
  }
}

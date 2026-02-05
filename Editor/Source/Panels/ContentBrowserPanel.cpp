#include "ContentBrowserPanel.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Project/Project.h"
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>

ContentBrowserPanel::ContentBrowserPanel(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_ListWidget = new QListWidget(this);
  m_ListWidget->setViewMode(QListView::IconMode);
  m_ListWidget->setIconSize(QSize(64, 64));
  m_ListWidget->setGridSize(QSize(100, 100));
  m_ListWidget->setResizeMode(QListView::Adjust);
  m_ListWidget->setContextMenuPolicy(Qt::CustomContextMenu);

  layout->addWidget(m_ListWidget);

  connect(m_ListWidget, &QListWidget::itemDoubleClicked, this,
          &ContentBrowserPanel::OnItemDoubleClicked);
  connect(m_ListWidget, &QListWidget::customContextMenuRequested, this,
          &ContentBrowserPanel::OnContextMenu);

  m_ListWidget->setAcceptDrops(true);
  m_ListWidget->setDragEnabled(true);
  m_ListWidget->setDragDropMode(QAbstractItemView::DragDrop);
  m_ListWidget->setDefaultDropAction(Qt::CopyAction);
  m_ListWidget->installEventFilter(this);

  Refresh();
}

bool ContentBrowserPanel::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_ListWidget) {
    if (event->type() == QEvent::DragEnter) {
      QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
      if (dragEvent->mimeData()->hasUrls()) {
        dragEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::DragMove) {
      QDragMoveEvent *dragEvent = static_cast<QDragMoveEvent *>(event);
      if (dragEvent->mimeData()->hasUrls()) {
        dragEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::Drop) {
      QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
      const QMimeData *mimeData = dropEvent->mimeData();

      if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl &url : urlList) {
          std::filesystem::path sourcePath = url.toLocalFile().toStdString();
          std::filesystem::path filename = sourcePath.filename();
          std::filesystem::path destPath = m_CurrentDirectory / filename;

          try {
            if (std::filesystem::exists(destPath)) {
              std::filesystem::copy_file(
                  sourcePath, destPath,
                  std::filesystem::copy_options::overwrite_existing);
            } else {
              std::filesystem::copy_file(sourcePath, destPath);
            }

            // Register with AssetManager
            Horse::AssetManager::Get().ImportAsset(destPath);

          } catch (std::exception &e) {
            QMessageBox::warning(
                this, "Import Error",
                QString("Failed to import %1:\n%2")
                    .arg(QString::fromStdString(filename.string()))
                    .arg(e.what()));
          }
        }
        Refresh();
        dropEvent->acceptProposedAction();
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void ContentBrowserPanel::Refresh() {
  m_ListWidget->clear();

  auto project = Horse::Project::GetActive();
  if (!project) {
    m_CurrentDirectory = "";
    m_BaseDirectory = "";
    return;
  }

  if (m_BaseDirectory.empty()) {
    m_BaseDirectory = Horse::Project::GetAssetDirectory();
    m_CurrentDirectory = m_BaseDirectory;
  }

  if (!std::filesystem::exists(m_CurrentDirectory)) {
    return;
  }

  QFileIconProvider iconProvider;

  // Add back button if not in base directory
  if (m_CurrentDirectory != m_BaseDirectory) {
    QListWidgetItem *item = new QListWidgetItem("..");
    item->setData(Qt::UserRole, QString(".."));
    item->setIcon(iconProvider.icon(QFileIconProvider::Folder));
    m_ListWidget->addItem(item);
  }

  for (const auto &entry :
       std::filesystem::directory_iterator(m_CurrentDirectory)) {
    QString name = QString::fromStdString(entry.path().filename().string());
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, QString::fromStdString(entry.path().string()));

    QFileInfo fileInfo(QString::fromStdString(entry.path().string()));
    item->setIcon(iconProvider.icon(fileInfo));

    m_ListWidget->addItem(item);
  }
}

void ContentBrowserPanel::OnItemDoubleClicked(QListWidgetItem *item) {
  QString pathStr = item->data(Qt::UserRole).toString();

  if (pathStr == "..") {
    m_CurrentDirectory = m_CurrentDirectory.parent_path();
    Refresh();
  } else {
    std::filesystem::path path(pathStr.toStdString());
    if (std::filesystem::is_directory(path)) {
      m_CurrentDirectory = path;
      Refresh();
    } else {
      // Open file in associated program
      QDesktopServices::openUrl(QUrl::fromLocalFile(pathStr));
    }
  }
}

void ContentBrowserPanel::OnContextMenu(const QPoint &pos) {
  QMenu menu(this);

  QListWidgetItem *item = m_ListWidget->itemAt(pos);

  if (item && item->text() != "..") {
    menu.addAction("Rename", this, &ContentBrowserPanel::OnRenameItem);
    menu.addAction("Delete", this, &ContentBrowserPanel::OnDeleteItem);
    menu.addSeparator();
  }

  menu.addAction("Import...", this, [this]() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Import Assets", "",
        "All Files (*.*);;Images (*.png *.jpg *.jpeg *.tga);;Models (*.obj "
        "*.fbx *.gltf *.glb)");
    for (const QString &file : files) {
      std::filesystem::path sourcePath = file.toStdString();
      std::filesystem::path destPath =
          m_CurrentDirectory / sourcePath.filename();
      try {
        if (std::filesystem::exists(destPath)) {
          std::filesystem::copy_file(
              sourcePath, destPath,
              std::filesystem::copy_options::overwrite_existing);
        } else {
          std::filesystem::copy_file(sourcePath, destPath);
        }
        Horse::AssetManager::Get().ImportAsset(destPath);
      } catch (...) {
      }
    }
    Refresh();
  });
  menu.addSeparator();

  menu.addAction("Create Folder", this, &ContentBrowserPanel::OnCreateFolder);
  menu.addAction("Create Material", this,
                 &ContentBrowserPanel::OnCreateMaterial);

  menu.exec(m_ListWidget->mapToGlobal(pos));
}

#include "HorseEngine/Render/Material.h"
#include "HorseEngine/Render/MaterialSerializer.h"

void ContentBrowserPanel::OnCreateMaterial() {
  bool ok;
  QString name = QInputDialog::getText(this, "New Material",
                                       "Material Name:", QLineEdit::Normal,
                                       "NewMaterial", &ok);

  if (ok && !name.isEmpty()) {
    std::string filename = name.toStdString();
    if (filename.find(".horsemat") == std::string::npos) {
      filename += ".horsemat";
    }
    std::filesystem::path newPath = m_CurrentDirectory / filename;

    if (!std::filesystem::exists(newPath)) {
      // Create new material
      Horse::Material material(name.toStdString());
      // Default values
      material.SetColor("Albedo", {1.0f, 1.0f, 1.0f, 1.0f});
      material.SetFloat("Roughness", 0.5f);
      material.SetFloat("Metalness", 0.0f);

      // Serialize
      if (Horse::MaterialSerializer::Serialize(material, newPath.string())) {
        Refresh();
      } else {
        QMessageBox::critical(this, "Error", "Failed to save material file.");
      }
    } else {
      QMessageBox::warning(this, "Error", "Material already exists!");
    }
  }
}

void ContentBrowserPanel::OnCreateFolder() {
  bool ok;
  QString name = QInputDialog::getText(
      this, "New Folder", "Folder Name:", QLineEdit::Normal, "NewFolder", &ok);

  if (ok && !name.isEmpty()) {
    std::filesystem::path newPath = m_CurrentDirectory / name.toStdString();
    if (!std::filesystem::exists(newPath)) {
      std::filesystem::create_directory(newPath);
      Refresh();
    } else {
      QMessageBox::warning(this, "Error", "Folder already exists!");
    }
  }
}

void ContentBrowserPanel::OnRenameItem() {
  QListWidgetItem *item = m_ListWidget->currentItem();
  if (!item || item->text() == "..")
    return;

  QString oldPathStr = item->data(Qt::UserRole).toString();
  std::filesystem::path oldPath(oldPathStr.toStdString());

  bool ok;
  QString newName = QInputDialog::getText(
      this, "Rename", "New Name:", QLineEdit::Normal, item->text(), &ok);

  if (ok && !newName.isEmpty() && newName != item->text()) {
    std::filesystem::path newPath =
        oldPath.parent_path() / newName.toStdString();
    try {
      std::filesystem::rename(oldPath, newPath);
      Refresh();
    } catch (const std::exception &e) {
      QMessageBox::critical(this, "Error",
                            QString("Failed to rename: %1").arg(e.what()));
    }
  }
}

void ContentBrowserPanel::OnDeleteItem() {
  QListWidgetItem *item = m_ListWidget->currentItem();
  if (!item || item->text() == "..")
    return;

  QString pathStr = item->data(Qt::UserRole).toString();
  std::filesystem::path path(pathStr.toStdString());

  auto result = QMessageBox::question(
      this, "Delete",
      QString("Are you sure you want to delete '%1'?").arg(item->text()),
      QMessageBox::Yes | QMessageBox::No);

  if (result == QMessageBox::Yes) {
    try {
      if (std::filesystem::is_directory(path)) {
        std::filesystem::remove_all(path);
      } else {
        std::filesystem::remove(path);
      }
      Refresh();
    } catch (const std::exception &e) {
      QMessageBox::critical(this, "Error",
                            QString("Failed to delete: %1").arg(e.what()));
    }
  }
}

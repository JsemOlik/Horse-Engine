#include "ContentBrowserPanel.h"
#include "HorseEngine/Project/Project.h"
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
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

  Refresh();
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

  // Add back button if not in base directory
  if (m_CurrentDirectory != m_BaseDirectory) {
    QListWidgetItem *item = new QListWidgetItem("..");
    item->setData(Qt::UserRole, QString(".."));
    // TODO: Add back icon
    m_ListWidget->addItem(item);
  }

  for (const auto &entry :
       std::filesystem::directory_iterator(m_CurrentDirectory)) {
    QString name = QString::fromStdString(entry.path().filename().string());
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, QString::fromStdString(entry.path().string()));

    if (entry.is_directory()) {
      // TODO: Add folder icon
    } else {
      // TODO: Add file icon
    }

    m_ListWidget->addItem(item);
  }
}

void ContentBrowserPanel::OnItemDoubleClicked(QListWidgetItem *item) {
  QString pathStr = item->data(Qt::UserRole).toString();

  if (pathStr == "..") {
    m_CurrentDirectory = m_CurrentDirectory.parent_path();
  } else {
    std::filesystem::path path(pathStr.toStdString());
    if (std::filesystem::is_directory(path)) {
      m_CurrentDirectory = path;
    }
  }

  Refresh();
}

void ContentBrowserPanel::OnContextMenu(const QPoint &pos) {
  QMenu menu(this);

  QListWidgetItem *item = m_ListWidget->itemAt(pos);

  if (item && item->text() != "..") {
    menu.addAction("Rename", this, &ContentBrowserPanel::OnRenameItem);
    menu.addAction("Delete", this, &ContentBrowserPanel::OnDeleteItem);
    menu.addSeparator();
  }

  menu.addAction("Create Folder", this, &ContentBrowserPanel::OnCreateFolder);

  menu.exec(m_ListWidget->mapToGlobal(pos));
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

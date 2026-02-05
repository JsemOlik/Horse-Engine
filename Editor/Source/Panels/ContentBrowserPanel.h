#pragma once

#include <QListWidget>
#include <QWidget>
#include <filesystem>

class QListWidgetItem;
class QDragEnterEvent;
class QDropEvent;

class ContentBrowserPanel : public QWidget {
  Q_OBJECT

public:
  explicit ContentBrowserPanel(QWidget *parent = nullptr);

  void Refresh();

  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void OnItemDoubleClicked(QListWidgetItem *item);
  void OnContextMenu(const QPoint &pos);
  void OnCreateFolder();
  void OnCreateMaterial();
  void OnRenameItem();
  void OnDeleteItem();

private:
  QListWidget *m_ListWidget;
  std::filesystem::path m_CurrentDirectory;
  std::filesystem::path m_BaseDirectory;
};

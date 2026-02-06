#pragma once

#include <QTreeWidget>
#include <QWidget>
#include <memory>

#include "HorseEngine/Scene/Entity.h"

namespace Horse {
class Scene;
} // namespace Horse

class HierarchyPanel : public QWidget {
  Q_OBJECT

public:
  explicit HierarchyPanel(QWidget *parent = nullptr);

  bool eventFilter(QObject *watched, QEvent *event) override;

  void SetScene(std::shared_ptr<Horse::Scene> scene);
  void RefreshHierarchy();

signals:
  void EntitySelected(Horse::Entity entity);

private slots:
  void OnItemSelectionChanged();
  void OnContextMenuRequested(const QPoint &pos);
  void OnCreateEmptyEntity();
  void OnCreateCube();

  void OnCreatePlayer();
  void OnCreateCamera();
  void OnCreateLight();
  void OnDeleteEntity();

private:
  void AddEntityToTree(Horse::Entity entity,
                       QTreeWidgetItem *parentItem = nullptr);

  QTreeWidget *m_TreeWidget;
  std::shared_ptr<Horse::Scene> m_Scene;
};

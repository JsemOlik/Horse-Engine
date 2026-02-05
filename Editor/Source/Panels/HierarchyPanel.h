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

  void SetScene(std::shared_ptr<Horse::Scene> scene);
  void RefreshHierarchy();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

signals:
  void EntitySelected(Horse::Entity entity);
  void CreatePrefabRequested(Horse::Entity entity);

private slots:
  void OnItemSelectionChanged();
  void OnContextMenuRequested(const QPoint &pos);
  void OnCreateEmptyEntity();
  void OnCreateCube();
  void OnCreateCamera();
  void OnCreateLight();
  void OnDeleteEntity();

private:
  void AddEntityToTree(Horse::Entity entity,
                       QTreeWidgetItem *parentItem = nullptr);

  QTreeWidget *m_TreeWidget;
  std::shared_ptr<Horse::Scene> m_Scene;
};

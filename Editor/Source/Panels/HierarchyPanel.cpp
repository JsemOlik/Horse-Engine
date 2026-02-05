#include "HierarchyPanel.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"

#include <QAction>
#include <QMenu>
#include <QVBoxLayout>

HierarchyPanel::HierarchyPanel(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_TreeWidget = new QTreeWidget(this);
  m_TreeWidget->setHeaderLabel("Scene");
  m_TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(m_TreeWidget, &QTreeWidget::itemSelectionChanged, this,
          &HierarchyPanel::OnItemSelectionChanged);
  connect(m_TreeWidget, &QTreeWidget::customContextMenuRequested, this,
          &HierarchyPanel::OnContextMenuRequested);

  layout->addWidget(m_TreeWidget);
}

void HierarchyPanel::SetScene(std::shared_ptr<Horse::Scene> scene) {
  m_Scene = scene;
  RefreshHierarchy();
}

void HierarchyPanel::RefreshHierarchy() {
  m_TreeWidget->clear();

  if (!m_Scene)
    return;

  // Get all entities and build tree
  auto &registry = m_Scene->GetRegistry();
  auto view = registry.view<Horse::TagComponent>();

  for (auto entity : view) {
    Horse::Entity ent(entity, m_Scene.get());

    // Only add root entities (entities without parents)
    if (ent.HasComponent<Horse::RelationshipComponent>()) {
      auto &rel = ent.GetComponent<Horse::RelationshipComponent>();
      if (rel.Parent == entt::null) {
        AddEntityToTree(ent);
      }
    } else {
      AddEntityToTree(ent);
    }
  }

  m_TreeWidget->expandAll();
}

void HierarchyPanel::AddEntityToTree(Horse::Entity entity,
                                     QTreeWidgetItem *parentItem) {
  if (!entity)
    return;

  auto &tag = entity.GetComponent<Horse::TagComponent>();

  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, QString::fromStdString(tag.Name));
  item->setData(0, Qt::UserRole, static_cast<quint32>(entity.GetHandle()));

  if (parentItem) {
    parentItem->addChild(item);
  } else {
    m_TreeWidget->addTopLevelItem(item);
  }

  // Add children
  if (entity.HasComponent<Horse::RelationshipComponent>()) {
    auto &rel = entity.GetComponent<Horse::RelationshipComponent>();
    entt::entity childHandle = rel.FirstChild;

    while (childHandle != entt::null) {
      Horse::Entity child(childHandle, m_Scene.get());
      AddEntityToTree(child, item);

      auto &childRel = child.GetComponent<Horse::RelationshipComponent>();
      childHandle = childRel.NextSibling;
    }
  }
}

void HierarchyPanel::OnItemSelectionChanged() {
  auto selectedItems = m_TreeWidget->selectedItems();
  if (selectedItems.isEmpty() || !m_Scene) {
    emit EntitySelected({});
    return;
  }

  quint32 entityHandle = selectedItems.first()->data(0, Qt::UserRole).toUInt();
  Horse::Entity entity(static_cast<entt::entity>(entityHandle), m_Scene.get());

  emit EntitySelected(entity);
}

void HierarchyPanel::OnContextMenuRequested(const QPoint &pos) {
  QMenu contextMenu(this);

  QAction *createEmptyAction = contextMenu.addAction("Create Empty Entity");
  connect(createEmptyAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateEmptyEntity);

  QAction *createCubeAction = contextMenu.addAction("Create Cube");
  connect(createCubeAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateCube);

  QAction *createCameraAction = contextMenu.addAction("Create Camera");
  connect(createCameraAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateCamera);

  QAction *createLightAction = contextMenu.addAction("Create Light");
  connect(createLightAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateLight);

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    contextMenu.addSeparator();

    QAction *createPrefabAction = contextMenu.addAction("Create Prefab");
    connect(createPrefabAction, &QAction::triggered, this, [this]() {
      auto selectedItems = m_TreeWidget->selectedItems();
      if (selectedItems.isEmpty() || !m_Scene)
        return;
      quint32 entityHandle =
          selectedItems.first()->data(0, Qt::UserRole).toUInt();
      Horse::Entity entity(static_cast<entt::entity>(entityHandle),
                           m_Scene.get());
      emit CreatePrefabRequested(entity);
    });

    QAction *deleteAction = contextMenu.addAction("Delete Entity");
    connect(deleteAction, &QAction::triggered, this,
            &HierarchyPanel::OnDeleteEntity);
  }

  contextMenu.exec(m_TreeWidget->mapToGlobal(pos));
}

void HierarchyPanel::OnCreateEmptyEntity() {
  if (!m_Scene)
    return;

  m_Scene->CreateEntity("Entity");
  RefreshHierarchy();
}

void HierarchyPanel::OnCreateCube() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Cube");
  auto &meshRenderer = entity.AddComponent<Horse::MeshRendererComponent>();
  meshRenderer.MaterialGUID = "";
  // Transform is added by default in CreateEntity (usually, checking
  // components.h)
  RefreshHierarchy();
}

void HierarchyPanel::OnCreateCamera() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Camera");
  entity.AddComponent<Horse::CameraComponent>();
  RefreshHierarchy();
}

void HierarchyPanel::OnCreateLight() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Light");
  entity.AddComponent<Horse::LightComponent>();
  RefreshHierarchy();
}

void HierarchyPanel::OnDeleteEntity() {
  auto selectedItems = m_TreeWidget->selectedItems();
  if (selectedItems.isEmpty() || !m_Scene)
    return;

  quint32 entityHandle = selectedItems.first()->data(0, Qt::UserRole).toUInt();
  Horse::Entity entity(static_cast<entt::entity>(entityHandle), m_Scene.get());

  m_Scene->DestroyEntity(entity);
  RefreshHierarchy();
  emit EntitySelected({});
}

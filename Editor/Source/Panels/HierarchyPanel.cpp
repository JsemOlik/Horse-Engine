#include "HierarchyPanel.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Entity.h"
#include "HorseEngine/Scene/Scene.h"

#include <QAction>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>
#include <filesystem>

#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Game/GameModule.h"

HierarchyPanel::HierarchyPanel(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_TreeWidget = new QTreeWidget(this);
  m_TreeWidget->setHeaderLabel("Scene");
  m_TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

  // Enable Drag and Drop
  m_TreeWidget->setDragEnabled(true);
  m_TreeWidget->setAcceptDrops(true);
  m_TreeWidget->setDragDropMode(QAbstractItemView::DragDrop);
  m_TreeWidget->setDefaultDropAction(Qt::MoveAction);
  m_TreeWidget->installEventFilter(this);
  m_TreeWidget->viewport()->installEventFilter(this);

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

bool HierarchyPanel::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_TreeWidget || watched == m_TreeWidget->viewport()) {
    if (event->type() == QEvent::DragEnter) {
      QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
      if (dragEvent->mimeData()->hasUrls() ||
          dragEvent->source() == m_TreeWidget) {
        dragEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::DragMove) {
      QDragMoveEvent *moveEvent = static_cast<QDragMoveEvent *>(event);
      if (moveEvent->mimeData()->hasUrls() ||
          moveEvent->source() == m_TreeWidget) {
        moveEvent->acceptProposedAction();
        return true;
      }
    } else if (event->type() == QEvent::Drop) {
      QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
      const QMimeData *mimeData = dropEvent->mimeData();

      // Handle Internal Move (Reparenting)
      if (dropEvent->source() == m_TreeWidget) {
        auto selectedItems = m_TreeWidget->selectedItems();
        QTreeWidgetItem *targetItem = m_TreeWidget->itemAt(dropEvent->pos());

        // Loop through all selected items to reparent them
        for (auto item : selectedItems) {
          quint32 childHandle = item->data(0, Qt::UserRole).toUInt();
          Horse::Entity child(static_cast<entt::entity>(childHandle),
                              m_Scene.get());

          if (targetItem) {
            // Reparent to target
            quint32 parentHandle = targetItem->data(0, Qt::UserRole).toUInt();
            Horse::Entity parent(static_cast<entt::entity>(parentHandle),
                                 m_Scene.get());

            // Avoid circular parenting or parenting to self (basic check)
            if (child != parent) {
              m_Scene->SetEntityParent(child, parent);
            }
          } else {
            // Reparent to Root (remove parent)
            m_Scene->SetEntityParent(child, {});
          }
        }
        RefreshHierarchy();
        dropEvent->acceptProposedAction();
        return true;
      }

      // Handle External Asset Drop (Prefabs/Meshes)
      if (mimeData->hasUrls()) {
        QTreeWidgetItem *targetItem = m_TreeWidget->itemAt(dropEvent->pos());
        Horse::Entity parentEntity;
        if (targetItem) {
          quint32 parentHandle = targetItem->data(0, Qt::UserRole).toUInt();
          parentEntity = Horse::Entity(static_cast<entt::entity>(parentHandle),
                                       m_Scene.get());
        }

        for (const QUrl &url : mimeData->urls()) {
          std::filesystem::path path = url.toLocalFile().toStdString();

          if (path.extension() == ".obj" || path.extension() == ".fbx" ||
              path.extension() == ".gltf" || path.extension() == ".glb") {
            auto entity = m_Scene->CreateEntity(path.stem().string());
            auto &renderer =
                entity.AddComponent<Horse::MeshRendererComponent>();
            auto &metadata = Horse::AssetManager::Get().GetMetadata(path);
            if (metadata.IsValid()) {
              renderer.MeshGUID = std::to_string((uint64_t)metadata.Handle);
            }

            if (parentEntity) {
              m_Scene->SetEntityParent(entity, parentEntity);
            }
          }
        }
        RefreshHierarchy();
        dropEvent->acceptProposedAction();
        return true;
      }
    }
  }
  return QObject::eventFilter(watched, event);
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
  item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

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

  QAction *createPlayerAction = contextMenu.addAction("Create Player");
  connect(createPlayerAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreatePlayer);

  QAction *createCameraAction = contextMenu.addAction("Create Camera");
  connect(createCameraAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateCamera);

  QAction *createLightAction = contextMenu.addAction("Create Light");
  connect(createLightAction, &QAction::triggered, this,
          &HierarchyPanel::OnCreateLight);

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    contextMenu.addSeparator();
    QAction *deleteAction = contextMenu.addAction("Delete Entity");
    connect(deleteAction, &QAction::triggered, this,
            &HierarchyPanel::OnDeleteEntity);
  }

  contextMenu.exec(m_TreeWidget->mapToGlobal(pos));
}

void HierarchyPanel::OnCreateEmptyEntity() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Empty Entity");

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    quint32 parentHandle =
        selectedItems.first()->data(0, Qt::UserRole).toUInt();
    Horse::Entity parent(static_cast<entt::entity>(parentHandle),
                         m_Scene.get());
    m_Scene->SetEntityParent(entity, parent);
  }

  RefreshHierarchy();
}

void HierarchyPanel::OnCreateCube() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Cube");

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    quint32 parentHandle =
        selectedItems.first()->data(0, Qt::UserRole).toUInt();
    Horse::Entity parent(static_cast<entt::entity>(parentHandle),
                         m_Scene.get());
    m_Scene->SetEntityParent(entity, parent);
  }

  auto &meshRenderer = entity.AddComponent<Horse::MeshRendererComponent>();
  meshRenderer.MaterialGUID = "";
  RefreshHierarchy();
}

void HierarchyPanel::OnCreatePlayer() {
  if (!m_Scene)
    return;

  // 1. Remove existing "Main Camera" if found
  auto &registry = m_Scene->GetRegistry();
  auto view = registry.view<Horse::TagComponent, Horse::CameraComponent>();
  for (auto entity : view) {
    auto &tag = view.get<Horse::TagComponent>(entity);
    if (tag.Name == "Main Camera") {
      m_Scene->DestroyEntity(Horse::Entity(entity, m_Scene.get()));
      // We can break if we assume only one, but let's be safe and catch
      // duplicate default cameras
    }
  }

  // 2. Create Player Entity
  auto player = m_Scene->CreateEntity("Player");

  // Set requested size (0.5 x 1.0 x 0.5)
  auto &playerTransform = player.GetComponent<Horse::TransformComponent>();
  playerTransform.Scale = {0.5f, 1.0f, 0.5f};

  // 2.5 Add MeshRenderer (so we can see it!)
  auto &meshRenderer = player.AddComponent<Horse::MeshRendererComponent>();
  meshRenderer.MaterialGUID = ""; // Defaults to standard material/cube mesh

  // 3. Add RigidBody
  auto &rb = player.AddComponent<Horse::RigidBodyComponent>();
  rb.Anchored = false; // Dynamic
  // rb.FixedRotation = true; // TODO: Add support for rotation locking in
  // RigidBodyComponent

  // 4. Add BoxCollider
  // Matches the visual scale (1.0 * 0.5 = 0.5 width, etc relative to physics?)
  // Jolt BoxShape uses Half-Extents usually, but our component defines full
  // Size? Let's assume Component.Size is Full Size in Local Space. Since we
  // scaled the Transform to 0.5, 1.0, 0.5. If Collider Size is 1,1,1 -> World
  // Size is 0.5, 1.0, 0.5. So we keep Collider Size at 1.0 (relative to
  // Transform) to match Mesh? Or is Collider Size absolute (World)? Usually
  // Physics shapes in ECS architectures are often independent or scaled by
  // transform. In Horse Engine Jolt Integration (PhysicsSystem.cpp), we need to
  // check if we apply scale. Assuming we propagate Scale to HalfExtents:
  //   HalfExtent = (Collider.Size * Transform.Scale) / 2.
  // So if we want total size 0.5, 1.0, 0.5...
  //   Transform 0.5, 1.0, 0.5 -> Collider Size 1.0, 1.0, 1.0 triggers correct
  //   shape.
  // Let's set Collider Size to {1.0f, 1.0f, 1.0f} so it matches the Mesh
  // perfectly.
  auto &bc = player.AddComponent<Horse::BoxColliderComponent>();
  bc.Size = {1.0f, 2.0f, 1.0f};
  bc.Offset = {0.0f, 0.0f, 0.0f}; // Centered

  // 5. Add Script
  // We need to use the Engine -> GameModule to create the script because we
  // can't link to Game DLL directly
  if (Horse::Engine::Get()->GetGameModule()) {
    Horse::Engine::Get()->GetGameModule()->CreateScript("PlayerController",
                                                        player);
  }

  // 6. Create Child Camera
  auto cameraEntity = m_Scene->CreateEntity("Camera");
  m_Scene->SetEntityParent(cameraEntity, player);

  auto &transform = cameraEntity.GetComponent<Horse::TransformComponent>();
  transform.Position = {0.0f, 1.6f, 0.0f}; // Eye height

  auto &camComp = cameraEntity.AddComponent<Horse::CameraComponent>();
  camComp.Primary = true;

  RefreshHierarchy();
}

void HierarchyPanel::OnCreateCamera() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Camera");

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    quint32 parentHandle =
        selectedItems.first()->data(0, Qt::UserRole).toUInt();
    Horse::Entity parent(static_cast<entt::entity>(parentHandle),
                         m_Scene.get());
    m_Scene->SetEntityParent(entity, parent);
  }

  entity.AddComponent<Horse::CameraComponent>();
  RefreshHierarchy();
}

void HierarchyPanel::OnCreateLight() {
  if (!m_Scene)
    return;

  auto entity = m_Scene->CreateEntity("Light");

  auto selectedItems = m_TreeWidget->selectedItems();
  if (!selectedItems.isEmpty()) {
    quint32 parentHandle =
        selectedItems.first()->data(0, Qt::UserRole).toUInt();
    Horse::Entity parent(static_cast<entt::entity>(parentHandle),
                         m_Scene.get());
    m_Scene->SetEntityParent(entity, parent);
  }

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

#pragma once

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>


#include "HorseEngine/Scene/Entity.h"

class InspectorPanel : public QWidget {
  Q_OBJECT

public:
  explicit InspectorPanel(QWidget *parent = nullptr);

  void SetSelectedEntity(Horse::Entity entity);

private:
  void RefreshInspector();
  void DrawComponents();

  QScrollArea *m_ScrollArea;
  QWidget *m_ContentWidget;
  QVBoxLayout *m_ContentLayout;
  Horse::Entity m_SelectedEntity;
};

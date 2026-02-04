#pragma once

#include <QWidget>
#include <QTreeWidget>

class HierarchyPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit HierarchyPanel(QWidget* parent = nullptr);
    
private:
    QTreeWidget* m_TreeWidget;
};

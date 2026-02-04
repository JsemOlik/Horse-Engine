#include "HierarchyPanel.h"
#include <QVBoxLayout>

HierarchyPanel::HierarchyPanel(QWidget* parent)
    : QWidget(parent) {
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_TreeWidget = new QTreeWidget(this);
    m_TreeWidget->setHeaderLabel("Scene");
    
    layout->addWidget(m_TreeWidget);
}

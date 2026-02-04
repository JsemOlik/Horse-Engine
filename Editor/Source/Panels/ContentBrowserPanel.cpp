#include "ContentBrowserPanel.h"
#include <QVBoxLayout>

ContentBrowserPanel::ContentBrowserPanel(QWidget* parent)
    : QWidget(parent) {
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_ListWidget = new QListWidget(this);
    
    layout->addWidget(m_ListWidget);
}

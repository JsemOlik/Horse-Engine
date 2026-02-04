#include "InspectorPanel.h"
#include <QVBoxLayout>

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent) {
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_PlaceholderLabel = new QLabel("Select an entity to inspect", this);
    m_PlaceholderLabel->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(m_PlaceholderLabel);
}

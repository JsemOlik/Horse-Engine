#include "ConsolePanel.h"
#include <QVBoxLayout>

ConsolePanel::ConsolePanel(QWidget* parent)
    : QWidget(parent) {
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_TextEdit = new QTextEdit(this);
    m_TextEdit->setReadOnly(true);
    m_TextEdit->setPlainText("Horse Engine Console\n");
    
    layout->addWidget(m_TextEdit);
}

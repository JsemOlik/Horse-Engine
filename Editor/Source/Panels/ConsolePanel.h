#pragma once

#include <QWidget>
#include <QTextEdit>

class ConsolePanel : public QWidget {
    Q_OBJECT
    
public:
    explicit ConsolePanel(QWidget* parent = nullptr);
    
private:
    QTextEdit* m_TextEdit;
};

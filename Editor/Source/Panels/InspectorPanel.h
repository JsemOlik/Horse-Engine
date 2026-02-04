#pragma once

#include <QWidget>
#include <QLabel>

class InspectorPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit InspectorPanel(QWidget* parent = nullptr);
    
private:
    QLabel* m_PlaceholderLabel;
};

#pragma once

#include <QWidget>
#include <QListWidget>

class ContentBrowserPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit ContentBrowserPanel(QWidget* parent = nullptr);
    
private:
    QListWidget* m_ListWidget;
};

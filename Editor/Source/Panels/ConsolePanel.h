#pragma once

#include <QTextEdit>
#include <QWidget>

class ConsolePanel : public QWidget {
  Q_OBJECT

public:
  explicit ConsolePanel(QWidget *parent = nullptr);
  ~ConsolePanel();

public slots:
  void AddLog(const QString &message, int level);
  void Clear();

private:
  QTextEdit *m_TextEdit;
  class QCheckBox *m_AutoScrollCheck;
  bool m_AutoScroll = true;
};

#pragma once

#include <QTextEdit>
#include <QWidget>

// Forward declare the sink type or use void*/template if circular deps issue?
// Better to include EditorLogSink.h in cpp and use forward decl here?
// But we used using alias `EditorLogSink_mt`.
// Let's forward declare the template class or just the alias?
// Simpler: Just allow setting it via method or pass in constructor?
// Let's pass in constructor.

class ConsolePanel : public QWidget {
  Q_OBJECT

public:
  explicit ConsolePanel(QWidget *parent = nullptr);
  ~ConsolePanel();
  void SetSink(std::shared_ptr<void> sink);

public slots:
  void AddLog(const QString &message, int level);
  void Clear();

private:
  QTextEdit *m_TextEdit;
  class QCheckBox *m_AutoScrollCheck;
  bool m_AutoScroll = true;
};

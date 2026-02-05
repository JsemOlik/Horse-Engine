#include "ConsolePanel.h"
#include "../EditorLogSink.h"
#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

// Removed internal QtLogSink implementation

ConsolePanel::ConsolePanel(QWidget *parent) : QWidget(parent) {

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);

  // Toolbar
  QHBoxLayout *toolbarData = new QHBoxLayout();
  toolbarData->setContentsMargins(4, 4, 4, 4);

  QPushButton *clearBtn = new QPushButton("Clear", this);
  connect(clearBtn, &QPushButton::clicked, this, &ConsolePanel::Clear);
  toolbarData->addWidget(clearBtn);

  m_AutoScrollCheck = new QCheckBox("Auto-scroll", this);
  m_AutoScrollCheck->setChecked(m_AutoScroll);
  connect(m_AutoScrollCheck, &QCheckBox::toggled,
          [this](bool checked) { m_AutoScroll = checked; });
  toolbarData->addWidget(m_AutoScrollCheck);

  toolbarData->addStretch();
  layout->addLayout(toolbarData);

  // Text Edit
  m_TextEdit = new QTextEdit(this);
  m_TextEdit->setReadOnly(true);
  // Dark background for console
  m_TextEdit->setStyleSheet(
      "font-family: Consolas, Monospace; font-size: 10pt; background-color: "
      "#1e1e1e; color: #d4d4d4; border: none;");
  layout->addWidget(m_TextEdit);
}

ConsolePanel::~ConsolePanel() {
  if (m_Sink) {
    auto castedSink = std::static_pointer_cast<EditorLogSink_mt>(m_Sink);
    if (castedSink) {
      castedSink->SetPanel(nullptr);
    }
  }
}

void ConsolePanel::SetSink(std::shared_ptr<void> sink) {
  m_Sink = sink;
  auto castedSink = std::static_pointer_cast<EditorLogSink_mt>(sink);
  if (castedSink) {
    castedSink->SetPanel(this);
  }
}

void ConsolePanel::AddLog(const QString &message, int level) {
  QString color = "#d4d4d4";
  spdlog::level::level_enum lvl = static_cast<spdlog::level::level_enum>(level);

  switch (lvl) {
  case spdlog::level::trace:
    color = "#808080";
    break;
  case spdlog::level::debug:
    color = "#569cd6";
    break;
  case spdlog::level::info:
    color = "#d4d4d4";
    break;
  case spdlog::level::warn:
    color = "#dcdcaa";
    break;
  case spdlog::level::err:
    color = "#f44747";
    break;
  case spdlog::level::critical:
    color = "#c586c0";
    break;
  default:
    break;
  }

  QString escapedMsg = message.toHtmlEscaped();
  escapedMsg.replace("\n", "<br>");

  QString formattedHtml =
      QString("<span style='color:%1'>%2</span><br>").arg(color, escapedMsg);

  m_TextEdit->moveCursor(QTextCursor::End);
  m_TextEdit->insertHtml(formattedHtml);

  if (m_AutoScroll) {
    m_TextEdit->verticalScrollBar()->setValue(
        m_TextEdit->verticalScrollBar()->maximum());
  }
}

void ConsolePanel::Clear() { m_TextEdit->clear(); }

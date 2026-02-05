#include "ConsolePanel.h"
#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>


#include "HorseEngine/Core/Logging.h"
#include <mutex>
#include <spdlog/sinks/base_sink.h>


// --- QtLogSink Implementation ---
template <typename Mutex>
class QtLogSink : public spdlog::sinks::base_sink<Mutex> {
public:
  QtLogSink(ConsolePanel *panel) : m_Panel(panel) {}

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    if (!m_Panel)
      return;

    spdlog::memory_buf_t formatted;
    this->formatter_->format(msg, formatted);
    std::string text = fmt::to_string(formatted);

    // Invoke on UI thread
    QString qText = QString::fromStdString(text);

    QMetaObject::invokeMethod(m_Panel, "AddLog", Qt::QueuedConnection,
                              Q_ARG(QString, qText),
                              Q_ARG(int, static_cast<int>(msg.level)));
  }

  void flush_() override {}

private:
  ConsolePanel *m_Panel;
};

using QtLogSink_mt = QtLogSink<std::mutex>;

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

  // Initialize Sink
  auto sink = std::make_shared<QtLogSink_mt>(this);
  sink->set_pattern("[%H:%M:%S] [%n] %v");

  // Attach to Loggers
  auto coreLogger = Horse::Logger::GetLogger(Horse::LogChannel::Core);
  if (coreLogger)
    coreLogger->sinks().push_back(sink);

  auto renderLogger = Horse::Logger::GetLogger(Horse::LogChannel::Render);
  if (renderLogger)
    renderLogger->sinks().push_back(sink);

  auto scriptLogger = Horse::Logger::GetLogger(Horse::LogChannel::Script);
  if (scriptLogger)
    scriptLogger->sinks().push_back(sink);

  auto assetLogger = Horse::Logger::GetLogger(Horse::LogChannel::Asset);
  if (assetLogger)
    assetLogger->sinks().push_back(sink);
}

ConsolePanel::~ConsolePanel() {}

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

  // Escape HTML special characters in message to avoid breaking layout
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

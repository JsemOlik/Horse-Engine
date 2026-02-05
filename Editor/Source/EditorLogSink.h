#pragma once

#include "Panels/ConsolePanel.h"
#include <QMetaObject>
#include <QString>
#include <mutex>
#include <spdlog/sinks/base_sink.h>
#include <string>
#include <vector>


// Forward declaration if needed, but we include ConsolePanel.h for invokeMethod
// safety or just pointer
class ConsolePanel;

template <typename Mutex>
class EditorLogSink : public spdlog::sinks::base_sink<Mutex> {
public:
  EditorLogSink() = default;

  void SetPanel(ConsolePanel *panel) {
    std::lock_guard<Mutex> lock(this->mutex_);
    m_Panel = panel;
    if (m_Panel) {
      FlushBuffer();
    }
  }

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    this->formatter_->format(msg, formatted);
    std::string text = fmt::to_string(formatted);
    int level = static_cast<int>(msg.level);

    // If locked, we might block the logging thread (Engine thread) waiting for
    // UI? No, we just acquired the mutex for this sink.

    if (m_Panel) {
      QString qText = QString::fromStdString(text);
      QMetaObject::invokeMethod(m_Panel, "AddLog", Qt::QueuedConnection,
                                Q_ARG(QString, qText), Q_ARG(int, level));
    } else {
      m_Buffer.emplace_back(level, text);
    }
  }

  void flush_() override {}

private:
  void FlushBuffer() {
    for (const auto &pair : m_Buffer) {
      int level = pair.first;
      std::string text = pair.second;
      QString qText = QString::fromStdString(text);

      QMetaObject::invokeMethod(m_Panel, "AddLog", Qt::QueuedConnection,
                                Q_ARG(QString, qText), Q_ARG(int, level));
    }
    m_Buffer.clear();
  }

  ConsolePanel *m_Panel = nullptr;
  std::vector<std::pair<int, std::string>> m_Buffer;
};

using EditorLogSink_mt = EditorLogSink<std::mutex>;

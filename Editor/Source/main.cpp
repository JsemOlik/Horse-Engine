#include "EditorLogSink.h"
#include "EditorWindow.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Core/Time.h"

#include <QApplication>

#include "HorseEngine/Engine.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Initialize Logger first
  Horse::Logger::Initialize();

  // Create sink to capture startup logs
  auto sink = std::make_shared<EditorLogSink_mt>();
  sink->set_pattern("[%H:%M:%S] [%n] %v");

  // Attach sink
  Horse::Logger::AddSink(sink);

  // Initialize Engine in Headless mode (Editor handles the window)
  Horse::Engine engine;
  engine.Initialize(true);

  HORSE_LOG_CORE_INFO("Horse Engine Editor v0.1.0");

  EditorWindow window(sink);
  window.show();

  int result = app.exec();

  engine.Shutdown();

  return result;
}

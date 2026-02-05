
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"


#include <iostream>

using namespace Horse;

class MyGameModule : public GameModule {
public:
  virtual void OnLoad() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnLoad() called!");
  }

  virtual void OnShutdown() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnShutdown() called!");
  }

  virtual void OnUpdate(float deltaTime) override {
    // Simple log throttling or just log every frame for debug?
    // Let's not log every frame to avoid spam, maybe just once in a while or
    // implement a simple counter.
    // For now, no log in Update to keep console clean.
  }
};

extern "C" {
__declspec(dllexport) GameModule *CreateGameModule() {
  return new MyGameModule();
}
}

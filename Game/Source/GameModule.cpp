
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"
#include "HorseEngine/Scene/ScriptableEntity.h"

#include <cmath>
#include <iostream>

using namespace Horse;

class VerticalMover : public ScriptableEntity {
public:
  virtual void OnUpdate(float deltaTime) override {
    auto &transform = GetComponent<TransformComponent>();
    m_Time += deltaTime;
    transform.Position[1] = std::sin(m_Time) * 5.0f;
  }

private:
  float m_Time = 0.0f;
};

class MyGameModule : public GameModule {
public:
  virtual void OnLoad() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnLoad() called!");
  }

  virtual void OnShutdown() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnShutdown() called!");
  }

  virtual void OnUpdate(float deltaTime) override {}

  virtual std::vector<std::string> GetAvailableScripts() const override {
    return {"VerticalMover"};
  }

  virtual void CreateScript(const std::string &name, Entity entity) override {
    if (name == "VerticalMover") {
      NativeScriptComponent *nsc = nullptr;
      if (entity.HasComponent<NativeScriptComponent>()) {
        nsc = &entity.GetComponent<NativeScriptComponent>();
      } else {
        nsc = &entity.AddComponent<NativeScriptComponent>();
      }

      nsc->Bind<VerticalMover>();
      nsc->ClassName = "VerticalMover";
    }
  }
};

extern "C" {
__declspec(dllexport) GameModule *CreateGameModule() {
  return new MyGameModule();
}
}

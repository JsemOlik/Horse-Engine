
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Scene/Components.h"
#include "HorseEngine/Scene/Scene.h"
#include "HorseEngine/Scene/ScriptableEntity.h"
#include "PlayerController.h"

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

#include "HorseEngine/Project/Project.h"
#include "HorseEngine/Scene/SceneSerializer.h"

// ...

class MyGameModule : public GameModule {
public:
  virtual void OnLoad() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnLoad() called!");

    // Load Scene
    // Retrieve default scene from Project Config
    std::string scenePath;
    if (auto project = Project::GetActive()) {
      scenePath = project->GetConfig().DefaultScene;
    }

    if (scenePath.empty()) {
      HORSE_LOG_CORE_WARN("No default scene specified in project config!");
      return;
    }

    m_ActiveScene = SceneSerializer::DeserializeFromJSON(scenePath);
    if (m_ActiveScene) {
      HORSE_LOG_CORE_INFO("Successfully loaded scene: {}", scenePath);

      // Re-bind native scripts
      auto view = m_ActiveScene->GetRegistry().view<NativeScriptComponent>();
      for (auto entity : view) {
        auto &nsc = view.get<NativeScriptComponent>(entity);
        if (!nsc.ClassName.empty()) {
          HORSE_LOG_CORE_INFO("Re-binding script: {} to entity", nsc.ClassName);
          CreateScript(nsc.ClassName, {entity, m_ActiveScene.get()});
        }
      }

      m_ActiveScene->OnRuntimeStart();

      // Resize logic removed as Scene doesn't support it directly yet.
    } else {
      HORSE_LOG_CORE_ERROR("Failed to load scene: {}", scenePath);
    }
  }

  virtual void OnShutdown() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnShutdown() called!");
    if (m_ActiveScene) {
      m_ActiveScene->OnRuntimeStop();
      m_ActiveScene = nullptr;
    }
  }

  virtual void OnUpdate(float deltaTime) override {
    if (m_ActiveScene) {
      m_ActiveScene->OnUpdate(deltaTime);
    }
  }

  virtual Scene *GetActiveScene() override { return m_ActiveScene.get(); }

private:
  std::shared_ptr<Scene> m_ActiveScene;

public:
  // ... rest

  virtual std::vector<std::string> GetAvailableScripts() const override {
    return {"VerticalMover", "PlayerController"};
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
    } else if (name == "PlayerController") {
      NativeScriptComponent *nsc = nullptr;
      if (entity.HasComponent<NativeScriptComponent>()) {
        nsc = &entity.GetComponent<NativeScriptComponent>();
      } else {
        nsc = &entity.AddComponent<NativeScriptComponent>();
      }
      nsc->Bind<PlayerController>();
      nsc->ClassName = "PlayerController";
    }
  }
};

extern "C" {
__declspec(dllexport) GameModule *CreateGameModule() {
  return new MyGameModule();
}
}

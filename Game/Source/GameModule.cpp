
#include "HorseEngine/Game/GameModule.h"
#include "HorseEngine/Asset/AssetManager.h"
#include "HorseEngine/Core/Logging.h"
#include "HorseEngine/Engine.h"
#include "HorseEngine/Project/Project.h"
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

#include "HorseEngine/Scene/SceneSerializer.h"

// ...

class MyGameModule : public GameModule {
public:
  virtual void OnLoad() override {
    HORSE_LOG_CORE_INFO("MyGameModule::OnLoad() called!");

    // Load Scene
    // Path should match what is in PAK.
    // Cooker output might be "Scenes/..." or just root relative.
    // Try scanning or hardcoded.
    // Load Scene via Project Config
    std::string scenePath = "Scenes/TC1.horselevel.horselevel";

    if (auto project = Project::GetActive()) {
      uint64_t defaultGUID = project->GetConfig().DefaultLevelGUID;
      HORSE_LOG_CORE_INFO("Active project found. DefaultLevelGUID: {}",
                          defaultGUID);
      if (defaultGUID != 0) {
        std::filesystem::path resolvedPath =
            AssetManager::Get().GetFileSystemPath(UUID(defaultGUID));
        if (!resolvedPath.empty()) {
          scenePath = resolvedPath.string();
          HORSE_LOG_CORE_INFO("Resolved default scene path: {}", scenePath);
          // Normalize separators for PhysFS/SceneSerializer
          std::replace(scenePath.begin(), scenePath.end(), '\\', '/');
        } else {
          HORSE_LOG_CORE_ERROR(
              "Failed to resolve file system path for GUID: {}", defaultGUID);
        }
      }
    } else {
      HORSE_LOG_CORE_WARN(
          "No active project found, using fallback scene path.");
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

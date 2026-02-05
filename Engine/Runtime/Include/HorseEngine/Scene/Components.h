#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "HorseEngine/Scene/UUID.h"
#include <array>
#include <entt/entt.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>


namespace Horse {

// UUID Component - Every entity has a unique identifier
struct UUIDComponent {
  UUID ID;

  UUIDComponent() = default;
  UUIDComponent(const UUID &uuid) : ID(uuid) {}
};

struct TagComponent {
  std::string Name = "Entity";
  std::string Tag = "Default";

  TagComponent() = default;
  TagComponent(const std::string &name) : Name(name) {}
};

struct TransformComponent {
  std::array<float, 3> Position = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> Rotation = {0.0f, 0.0f, 0.0f}; // Euler angles in degrees
  std::array<float, 3> Scale = {1.0f, 1.0f, 1.0f};

  glm::mat4 WorldTransform = glm::mat4(1.0f);

  TransformComponent() = default;
};

struct RelationshipComponent {
  entt::entity Parent = entt::null;
  entt::entity FirstChild = entt::null;
  entt::entity NextSibling = entt::null;
  entt::entity PrevSibling = entt::null;

  RelationshipComponent() = default;
};

struct CameraComponent {
  enum class ProjectionType { Perspective = 0, Orthographic = 1 };

  ProjectionType Type = ProjectionType::Perspective;
  float FOV = 45.0f;
  float NearClip = 0.1f;
  float FarClip = 1000.0f;
  float OrthographicSize = 10.0f;
  bool Primary = true;

  CameraComponent() = default;
};

struct LightComponent {
  enum class LightType { Directional = 0, Point = 1, Spot = 2 };

  LightType Type = LightType::Directional;
  std::array<float, 3> Color = {1.0f, 1.0f, 1.0f};
  float Intensity = 1.0f;
  float Range = 10.0f;     // For point/spot lights
  float SpotAngle = 45.0f; // For spot lights

  LightComponent() = default;
};

struct MeshRendererComponent {
  std::string MeshGUID;
  std::string MaterialGUID;

  MeshRendererComponent() = default;
};

struct ScriptComponent {
  std::string ScriptGUID;
  bool AwakeCalled = false;
  bool StartCalled = false;

  ScriptComponent() = default;
};

} // namespace Horse

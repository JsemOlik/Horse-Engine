#pragma once

#include "HorseEngine/Core.h"
#include "HorseEngine/Scene/UUID.h"
#include <entt/entt.hpp>
#include <string>


namespace Horse {

struct TagComponent {
  std::string Name = "Entity";
  std::string Tag;

  TagComponent() = default;
  TagComponent(const std::string &name) : Name(name) {}
};

struct TransformComponent {
  float Position[3] = {0.0f, 0.0f, 0.0f};
  float Rotation[3] = {0.0f, 0.0f, 0.0f}; // Euler angles in degrees
  float Scale[3] = {1.0f, 1.0f, 1.0f};

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
  float Color[3] = {1.0f, 1.0f, 1.0f};
  float Intensity = 1.0f;
  float Range = 10.0f;     // For point/spot lights
  float SpotAngle = 45.0f; // For spot lights

  LightComponent() = default;
};

struct MeshRendererComponent {
  UUID MeshID;
  UUID MaterialID;

  MeshRendererComponent() = default;
};

} // namespace Horse

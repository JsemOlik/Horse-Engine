#pragma once

#include <DirectXMath.h>
#include <array>
#include <vector>

namespace Horse {

struct AABB {
  DirectX::XMFLOAT3 Center;
  DirectX::XMFLOAT3 Extents; // Half-width, half-height, half-depth
};

struct Plane {
  DirectX::XMFLOAT3 Normal;
  float Distance;

  void Normalize() {
    float length =
        sqrtf(Normal.x * Normal.x + Normal.y * Normal.y + Normal.z * Normal.z);
    Normal.x /= length;
    Normal.y /= length;
    Normal.z /= length;
    Distance /= length;
  }
};

class Frustum {
public:
  Frustum() = default;

  void Update(const DirectX::XMMATRIX &viewProj) {
    using namespace DirectX;
    XMFLOAT4X4 vp;
    XMStoreFloat4x4(&vp, viewProj);

    // Left plane
    m_Planes[0].Normal.x = vp._14 + vp._11;
    m_Planes[0].Normal.y = vp._24 + vp._21;
    m_Planes[0].Normal.z = vp._34 + vp._31;
    m_Planes[0].Distance = vp._44 + vp._41;
    m_Planes[0].Normalize();

    // Right plane
    m_Planes[1].Normal.x = vp._14 - vp._11;
    m_Planes[1].Normal.y = vp._24 - vp._21;
    m_Planes[1].Normal.z = vp._34 - vp._31;
    m_Planes[1].Distance = vp._44 - vp._41;
    m_Planes[1].Normalize();

    // Bottom plane
    m_Planes[2].Normal.x = vp._14 + vp._12;
    m_Planes[2].Normal.y = vp._24 + vp._22;
    m_Planes[2].Normal.z = vp._34 + vp._32;
    m_Planes[2].Distance = vp._44 + vp._42;
    m_Planes[2].Normalize();

    // Top plane
    m_Planes[3].Normal.x = vp._14 - vp._12;
    m_Planes[3].Normal.y = vp._24 - vp._22;
    m_Planes[3].Normal.z = vp._34 - vp._32;
    m_Planes[3].Distance = vp._44 - vp._42;
    m_Planes[3].Normalize();

    // Near plane
    m_Planes[4].Normal.x = vp._13;
    m_Planes[4].Normal.y = vp._23;
    m_Planes[4].Normal.z = vp._33;
    m_Planes[4].Distance = vp._43;
    m_Planes[4].Normalize();

    // Far plane
    m_Planes[5].Normal.x = vp._14 - vp._13;
    m_Planes[5].Normal.y = vp._24 - vp._23;
    m_Planes[5].Normal.z = vp._34 - vp._33;
    m_Planes[5].Distance = vp._44 - vp._43;
    m_Planes[5].Normalize();
  }

  bool Intersects(const AABB &aabb) const {
    using namespace DirectX;

    for (const auto &plane : m_Planes) {
      XMVECTOR planeNormal = XMLoadFloat3(&plane.Normal);
      XMVECTOR boxCenter = XMLoadFloat3(&aabb.Center);
      XMVECTOR boxExtents = XMLoadFloat3(&aabb.Extents);

      // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
      XMVECTOR r = XMVector3Dot(XMVectorAbs(planeNormal), boxExtents);
      XMVECTOR d = XMVector3Dot(planeNormal, boxCenter);

      // Distance from box center to plane + radius < 0 means box is outside
      if (XMVectorGetX(d) + XMVectorGetX(r) + plane.Distance < 0.0f) {
        return false;
      }
    }
    return true;
  }

private:
  std::array<Plane, 6> m_Planes;
};

} // namespace Horse

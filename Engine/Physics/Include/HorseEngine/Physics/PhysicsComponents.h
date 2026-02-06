#pragma once

#include "HorseEngine/Core.h"
#include <array>

namespace Horse {

    struct RigidBodyComponent {
        bool Anchored = false;      // If true, body is Static. If false, Dynamic.
        bool UseGravity = true;     // Only applies if Dynamic
        bool IsSensor = false;      // If true, no physical collision response
        
        std::array<float, 3> LinearVelocity = { 0.0f, 0.0f, 0.0f };
        std::array<float, 3> AngularVelocity = { 0.0f, 0.0f, 0.0f };

        // Runtime data - Opaque pointer to Jolt Body
        void* RuntimeBody = nullptr; 

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent&) = default;
    };

    struct BoxColliderComponent {
        std::array<float, 3> Size = { 1.0f, 1.0f, 1.0f };
        std::array<float, 3> Offset = { 0.0f, 0.0f, 0.0f };
        
        // Placeholder for Physics Material asset
        // UUID MaterialID = 0; 

        BoxColliderComponent() = default;
        BoxColliderComponent(const BoxColliderComponent&) = default;
    };

}

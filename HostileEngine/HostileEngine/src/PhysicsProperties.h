#pragma once
#include "directxtk12/SimpleMath.h"
#include "Matrix3.h"

namespace Hostile {
    using DirectX::SimpleMath::Vector3;

    inline static double PHYSICS_TARGET_FPS_INV = 1 / 60.f;

    struct Rigidbody {};//tag

    struct Constraint { //plane (for now)
    };

    struct SphereCollider {
    };

    struct BoxCollider {
    };



    struct Velocity {
        Vector3 linear;
        Vector3 angular;
    };

    struct Acceleration {
        Vector3 linear;
        Vector3 angular;
    };

    struct Force {
        Vector3 force;//linear
        Vector3 torque;//angular
    };

    struct InertiaTensor {
        Matrix3 inverseInertiaTensor;
        Matrix3 inverseInertiaTensorWorld;
    };

    struct MassProperties {
        float inverseMass;
        MassProperties(float mass = 1.f)
        {
            assert(mass != 0.0f && "Mass can't be zero");
            inverseMass = 1.f / mass;
        }
    };
}
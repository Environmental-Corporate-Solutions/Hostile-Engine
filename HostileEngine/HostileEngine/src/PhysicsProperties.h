#pragma once
#include "directxtk12/SimpleMath.h"
#include "Matrix3.h"

namespace Hostile {
    using DirectX::SimpleMath::Vector3;

    inline static double PHYSICS_TARGET_FPS_INV = 1 / 60.f;

    struct Rigidbody {
        Matrix3 m_inverseInertiaTensor;
        Vector3 m_linearVelocity;
        Vector3 m_linearAcceleration;
        Vector3 m_angularVelocity;
        Vector3 m_angularAcceleration;
        Vector3 m_force; // Linear force
        Vector3 m_torque; // Angular force
        Matrix3 m_inverseInertiaTensorWorld;
        float m_inverseMass;
        float m_linearDamping;
        float m_angularDamping;
        bool m_useGravity;

        Rigidbody(
            const Matrix3& inverseInertiaTensor = Matrix3(),
            const Vector3& linearVelocity = Vector3(),
            const Vector3& linearAcceleration = Vector3(),
            const Vector3& angularVelocity = Vector3(),
            const Vector3& angularAcceleration = Vector3(),
            const Vector3& force = Vector3(),//linear force
            const Vector3& torque = Vector3(),//angular force
            float mass= 2.f,
            float linearDamping = 0.9f,
            float angularDamping=0.65f,
            bool useGravity=true)
            :
            m_inverseInertiaTensor(inverseInertiaTensor),
            m_linearVelocity(linearVelocity),
            m_linearAcceleration(linearAcceleration),
            m_angularVelocity(angularVelocity),
            m_angularAcceleration(angularAcceleration),
            m_force(force),
            m_torque(torque),
            m_inverseInertiaTensorWorld(Matrix3{}),
            m_inverseMass(mass != 0.0f ? 1.0f / mass : 0.0f),
            m_linearDamping(linearDamping),
            m_angularDamping(angularDamping),
            m_useGravity(useGravity)
        {
            assert(mass != 0.0f && "Mass can't be zero");
        }
    };


    struct Collider 
    {
        enum class Type 
        {
            Plane,
            Sphere,
            Box
        };

        bool m_isTrigger;
        Type m_colliderType;

        Collider(Type type, bool trigger = false) : m_colliderType(type), m_isTrigger(trigger){}

        virtual ~Collider() = default;
    };

    struct PlaneCollider : public Collider 
    {
        using Collider::Collider;
        PlaneCollider(bool trigger = false) : Collider(Type::Plane, trigger) {}
    };

    struct SphereCollider : public Collider 
    {
        using Collider::Collider;
        SphereCollider(bool trigger = false) : Collider(Type::Sphere, trigger) {}
    };

    struct BoxCollider : public Collider 
    {
        using Collider::Collider;
        BoxCollider(bool trigger = false) : Collider(Type::Box, trigger) {}
    };
}
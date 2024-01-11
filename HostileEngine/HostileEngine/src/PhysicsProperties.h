#pragma once
#include "directxtk12/SimpleMath.h"
#include "Matrix3.h"
#include <variant>

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
        bool m_isStatic;

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
            float angularDamping=0.1f,
            bool useGravity=true,
            bool isStatic=false)
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
            m_useGravity(useGravity),
            m_isStatic(isStatic)
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
        Vector3 m_offset;

        Collider(Type _type, bool _trigger, const Vector3& _offset) : m_colliderType(_type), m_isTrigger(_trigger), m_offset{_offset} {}

        virtual ~Collider() = default;

        template<typename T>
        void SetScale(const T& scale) {
            if constexpr (std::is_same_v<T, Vec3>) {
                static_cast<BoxCollider*>(this)->SetScaleInternal(scale);
            }
            else if constexpr (std::is_same_v<T, float>) {
                static_cast<SphereCollider*>(this)->SetScaleInternal(scale);
            }
        }

        [[nodiscard]] Vector3 GetOffset() const noexcept { return m_offset; }
        virtual SimpleMath::Matrix GetScaleMatrix() const = 0;

        // GetScale method that returns either a float or Vec3
        virtual std::variant<float, SimpleMath::Vector3> GetScale() const = 0;

    };

    struct PlaneCollider : public Collider 
    {
        using Collider::Collider;
        PlaneCollider(bool _trigger = false, const Vector3& _offset = Vector3{0.f,0.f,0.f}) : Collider(Type::Plane, _trigger, _offset) {}

        SimpleMath::Matrix GetScaleMatrix() const override final {
            return SimpleMath::Matrix{
                FLT_MAX, 0.f,    0.f,    0.f,
                0.f,    FLT_MAX, 0.f,    0.f,
                0.f,    0.f,    FLT_MAX, 0.f,
                0.f,    0.f,    0.f,    1.f
            };
        }

        std::variant<float, SimpleMath::Vector3> GetScale() const override final {
            return FLT_MAX;
        }
    };

    struct SphereCollider : public Collider 
    {
        using Collider::Collider;
        float radius;

        SphereCollider(bool _trigger = false, float _radius = 1.f, const Vector3& _offset = Vector3{0.f,0.f,0.f}) : radius{ _radius }, Collider(Type::Sphere, _trigger,_offset) {}

        void SetScaleInternal(float scale) {
            radius = scale;
        }
        SimpleMath::Matrix GetScaleMatrix() const override final {
			return SimpleMath::Matrix{
				radius, 0.f,    0.f,    0.f,
				0.f,    radius, 0.f,    0.f,
				0.f,    0.f,    radius, 0.f,
				0.f,    0.f,    0.f,    1.f
			};
        }

        std::variant<float, SimpleMath::Vector3> GetScale() const override final {
            return radius; // Returns float (radius) for SphereCollider
        }
    };

    struct BoxCollider : public Collider 
    {
        using Collider::Collider;
        SimpleMath::Vector3 scale; // the dimensions of the box
        BoxCollider(bool _trigger = false, const SimpleMath::Vector3& _scl = SimpleMath::Vector3{ 3.f,3.f,3.f }, const Vector3& _offset = Vector3{1.f,1.f,1.f}) : Collider(Type::Box, _trigger, _offset), scale{ _scl } {}

    public:
        void SetScaleInternal(const SimpleMath::Vector3& _scale) {
            scale = _scale;
        }
        SimpleMath::Matrix GetScaleMatrix() const override final {
            return SimpleMath::Matrix{
                scale.x, 0.f,    0.f,    0.f,
                0.f,    scale.y, 0.f,    0.f,
                0.f,    0.f,    scale.z, 0.f,
                0.f,    0.f,    0.f,    1.f
            };
        }


        std::variant<float, SimpleMath::Vector3> GetScale() const override final {
            return scale; // Returns Vec3 for BoxCollider
        }

    };
}
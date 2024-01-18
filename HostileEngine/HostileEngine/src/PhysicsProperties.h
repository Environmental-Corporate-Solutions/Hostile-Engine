#pragma once
#include "directxtk12/SimpleMath.h"
#include "Matrix3.h"
#include <variant>

namespace Hostile {
    using DirectX::SimpleMath::Vector3;

    inline static double PHYSICS_UPDATE_TARGET_FPS_INV = 1 / 120.f;

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
        bool m_lockRotationX;
        bool m_lockRotationY;
        bool m_lockRotationZ;

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
            float angularDamping=0.08f,
            bool useGravity=true,
            bool isStatic=false,
            bool lockRotationX = false,
            bool lockRotationY = false,
            bool lockRotationZ = false)
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
            m_isStatic(isStatic),
            m_lockRotationX{ lockRotationX },
            m_lockRotationY{ lockRotationY },
            m_lockRotationZ{ lockRotationZ }
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
        float m_restitution;
        float m_friction;
        Type m_colliderType;
        Vector3 m_offset;

        Collider(Type _type, bool _trigger, const Vector3& _offset, float _friction, float _restitution)
            : m_colliderType(_type), m_isTrigger(_trigger), m_offset{_offset} 
            , m_restitution{ _restitution }, m_friction{ _friction}
        {}

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

        [[nodiscard]]  virtual std::variant<float, SimpleMath::Vector3> GetScale() const = 0;

    };

    struct PlaneCollider : public Collider 
    {
        static constexpr float DEFAULT_PLANE_FRICTION = 0.9f;
        static constexpr float DEFAULT_PLANE_RESTITUTION = 0.f;

        PlaneCollider(bool _trigger = false, const Vector3& _offset = Vector3{0.f,0.f,0.f}) 
            : Collider(Type::Plane, _trigger, _offset, DEFAULT_PLANE_FRICTION, DEFAULT_PLANE_RESTITUTION) {}

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
        static constexpr float DEFAULT_SPHERE_FRICTION = 0.5f;
        static constexpr float DEFAULT_SPHERE_RESTITUTION = 0.7f;

        float radius;

        SphereCollider(bool _trigger = false, float _radius = 1.f, const Vector3& _offset = Vector3{0.f,0.f,0.f}) : radius{ _radius }, Collider(Type::Sphere, _trigger,_offset, DEFAULT_SPHERE_FRICTION, DEFAULT_SPHERE_RESTITUTION) {}

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
        static constexpr float DEFAULT_BOX_FRICTION = 0.1f;
        static constexpr float DEFAULT_BOX_RESTITUTION = 0.9f;

        SimpleMath::Vector3 m_scale; // the dimensions of the box
        BoxCollider(bool _trigger = false, const SimpleMath::Vector3& _scl = SimpleMath::Vector3{ 1.f,1.f,1.f }, const Vector3& _offset = Vector3{0.f,0.f,0.f})
            : Collider(Type::Box, _trigger, _offset, DEFAULT_BOX_FRICTION, DEFAULT_BOX_RESTITUTION), m_scale{ _scl } {}

        void SetScaleInternal(const SimpleMath::Vector3& _scale) {
            m_scale = _scale;
        }
        SimpleMath::Matrix GetScaleMatrix() const override final {
            return SimpleMath::Matrix{
                m_scale.x, 0.f,    0.f,    0.f,
                0.f,    m_scale.y, 0.f,    0.f,
                0.f,    0.f,    m_scale.z, 0.f,
                0.f,    0.f,    0.f,    1.f
            };
        }


        std::variant<float, SimpleMath::Vector3> GetScale() const override final {
            return m_scale; // Returns Vec3 for BoxCollider
        }

    };
}
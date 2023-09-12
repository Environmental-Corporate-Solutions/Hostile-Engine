#pragma once

#include "math/matrix3.h"
#include "DirectXTK/SimpleMath.h"


namespace physics
{
    using DirectX::SimpleMath::Matrix;
    using DirectX::SimpleMath::Vector2;
    using DirectX::SimpleMath::Vector3;
    using DirectX::SimpleMath::Quaternion;
    using math::Matrix3;

    struct RigidBody
    {
        Quaternion m_orientation;
        Vector3 m_position;
        Vector3 m_velocity;
        Vector3 m_angularVelocity;
        Vector3 m_linearAcceleration;
        Vector3 m_force;
        Vector3 m_torque;

        Matrix3 m_inverseInertiaTensor;       //local, constant
        Matrix3 m_inverseInertiaTensorWorld;  //world, need to be updated whenever rotates
        Matrix m_model;

        float m_inverseMass;

        float m_linearDamping;
        float m_angularDamping;

    //public:
        RigidBody() : m_inverseMass{0.f},m_linearDamping(0.95f), m_angularDamping(0.7f) {}

        void Integrate(float duration);
        void Rotate(const Quaternion&);
        Vector3 GetAxis(int index) const;

        void SetMass(float value);
        void SetInertiaTensor(const Matrix3& mat);
        void SetInverseInertiaTensor(const Matrix3& mat);
        void SetPosition(const Vector3& vec);
        void SetOrientation(const Quaternion&);
        void SetAngularVelocity(const Vector3& vec);

        float GetMass() const;
        Vector3 GetAngularVelocity() const;
    private:    
        void UpdateTransformMatrix();
        void UpdateInertiaTensor();
    }; 
}
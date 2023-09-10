#include "body.h"
#include "math/matrixConversion.h"
#include <cmath>
#include <iostream>

using namespace physics;

void RigidBody::Integrate(float dt)
{
    if (m_inverseMass == 0.0f) {
        return;
    }

    //1. linear Velocity
    Vector3 linearAcceleration = m_force * m_inverseMass;
    m_velocity += linearAcceleration * dt;
    m_velocity *= powf(m_linearDamping, dt);

    //2. angular Velocity
    Vector3 angularAcceleration = m_inverseInertiaTensorWorld * m_torque;
    m_angularVelocity += angularAcceleration * dt;
    m_angularVelocity *= powf(m_angularDamping, dt);

    //3.Pos, Orientation
    m_position += m_velocity * dt;
    {
        Vector3 scaledVelocity = m_angularVelocity * dt;
        float angle = scaledVelocity.Length();
        Vector3 axis = (fabs(angle) < 1e-6f) ? Vector3(0, 1, 0) : scaledVelocity / angle;
        Quaternion deltaRotation = Quaternion::CreateFromAxisAngle(axis, angle);
        m_orientation = deltaRotation * m_orientation;
    }
    m_orientation.Normalize();    

    //4.update accordingly
    UpdateTransformMatrix();
    UpdateInertiaTensor();

    //5.clear
    m_force = DirectX::SimpleMath::Vector3::Zero;
    m_torque = DirectX::SimpleMath::Vector3::Zero;
}


Vector3 RigidBody::GetAxis(int index) const
{
    if (index < 0 || index > 2){
        throw std::runtime_error("RigidBody::getAxis(), out of range\n");
    }

    Vector3 result(m_model.m[index][0],m_model.m[index][1],m_model.m[index][2]);
    result.Normalize();

    return result;
}

void RigidBody::Rotate(const Quaternion& quat)
{
    Quaternion newOrientation = m_orientation * quat;
    newOrientation.Normalize();
    SetOrientation(newOrientation);
}

void RigidBody::UpdateTransformMatrix()
{
    m_model.m[0][0] =
        1.0f - 2.0f * (m_orientation.y * m_orientation.y + m_orientation.z * m_orientation.z);
    m_model.m[1][0] =
        2.0f * (m_orientation.x * m_orientation.y - m_orientation.w * m_orientation.z);
    m_model.m[2][0] =
        2.0f * (m_orientation.x * m_orientation.z + m_orientation.w * m_orientation.y);

    m_model.m[0][1] =
        2.0f * (m_orientation.x * m_orientation.y + m_orientation.w * m_orientation.z);
    m_model.m[1][1] =
        1.0f - 2.0f * (m_orientation.x * m_orientation.x + m_orientation.z * m_orientation.z);
    m_model.m[2][1] =
        2.0f * (m_orientation.y * m_orientation.z - m_orientation.w * m_orientation.x);

    m_model.m[0][2] =
        2.0f * (m_orientation.x * m_orientation.z - m_orientation.w * m_orientation.y);
    m_model.m[1][2] =
        2.0f * (m_orientation.y * m_orientation.z + m_orientation.w * m_orientation.x);
    m_model.m[2][2] =
        1.0f - 2.0f * (m_orientation.x * m_orientation.x + m_orientation.y * m_orientation.y);

    m_model.m[3][0] = m_position.x;
    m_model.m[3][1] = m_position.y;
    m_model.m[3][2] = m_position.z;
}

void RigidBody::UpdateInertiaTensor()
{
    Matrix3 rotationMatrix = math::Extract3x3(m_model);
    m_inverseInertiaTensorWorld = (m_inverseInertiaTensor* rotationMatrix) * rotationMatrix.Transpose();
}

void RigidBody::SetMass(float value)
{
    m_inverseMass = 1.0f / value;
}


void RigidBody::SetInertiaTensor(const Matrix3& mat)
{
    m_inverseInertiaTensor = mat.Inverse();
    UpdateInertiaTensor();
}

void RigidBody::SetInverseInertiaTensor(const Matrix3& mat)
{
    m_inverseInertiaTensor = mat;
    UpdateInertiaTensor();
}

void RigidBody::SetPosition(const Vector3& vec)
{
    m_position = vec;
    UpdateTransformMatrix();
}

void RigidBody::SetOrientation(const Quaternion& quat)
{
    m_orientation = quat;
    UpdateTransformMatrix();
    UpdateInertiaTensor();
}

void RigidBody::SetAngularVelocity(const Vector3& vec){
    m_angularVelocity = math::Extract3x3(m_model).Transpose() * vec;
}

float RigidBody::GetMass() const{
    return 1.0f / m_inverseMass;
}

Vector3 RigidBody::GetAngularVelocity() const{
    return math::Extract3x3(m_model) * m_angularVelocity;
}

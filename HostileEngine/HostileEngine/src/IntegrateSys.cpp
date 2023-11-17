//------------------------------------------------------------------------------
//
// File Name:	IntegrateSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Engine.h"
#include "IntegrateSys.h"
#include "TransformSys.h"
#include "CollisionData.h"//collisionData
#include "PhysicsProperties.h"
#include <iostream>

namespace Hostile {

    ADD_SYSTEM(IntegrateSys);

    void IntegrateSys::OnCreate(flecs::world& _world)
    {
        _world.system<Transform,Rigidbody>("IntegrateSys")
            .rate(PHYSICS_TARGET_FPS_INV)
            .kind(IEngine::Get().GetIntegratePhase())
            .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("integrate phase place holder");
        //e.add<Force>();
    }

    void IntegrateSys::OnUpdate(flecs::iter& _it, Transform*_transform,Rigidbody* _rigidbody)
    {
        auto dt = _it.delta_time();
        size_t Cnt = _it.count();
        for (int i = 0; i < Cnt; i++) 
        {

            //if (_rigidbody->m_inverseMass == 0.f) {
            //    continue;
            //}
            // 1. Linear Velocity
            Vector3 linearAcceleration = _rigidbody[i].m_force * _rigidbody[i].m_inverseMass;
            _rigidbody[i].m_linearVelocity += linearAcceleration * dt;
            _rigidbody[i].m_linearVelocity *= powf(_rigidbody[i].m_drag, dt);

            // 2. Angular Velocity
            Vector3 angularAcceleration = { _rigidbody[i].m_inverseInertiaTensorWorld * _rigidbody[i].m_torque};
            _rigidbody[i].m_angularVelocity += angularAcceleration * dt;
            _rigidbody[i].m_angularVelocity *= powf(_rigidbody[i].m_angularDrag, dt);
            
            // 3. Calculate the new world position and orientation for the entity
            Transform worldTransform = TransformSys::GetWorldTransform(_it.entity(i));
            worldTransform.position += _rigidbody[i].m_linearVelocity * dt; 

            Quaternion deltaRotation = Quaternion::Identity;
            if (_rigidbody[i].m_angularVelocity.LengthSquared() > FLT_EPSILON) {
                Vector3 angularVelocityNormalized = _rigidbody[i].m_angularVelocity;//
                angularVelocityNormalized.Normalize();
                float angularSpeed = _rigidbody[i].m_angularVelocity.Length();
                deltaRotation = Quaternion::CreateFromAxisAngle(angularVelocityNormalized, angularSpeed * dt);
            }
            worldTransform.orientation = deltaRotation * worldTransform.orientation; // World orientation update
            worldTransform.orientation.Normalize();

            // 4. If the entity has a parent, calculate the local transform
            if (_it.entity(i).parent().is_valid()) {
                Transform parentWorldTransform = TransformSys::GetWorldTransform(_it.entity(i).parent());

                // Convert world position to parent-relative position
                Vector3 relativePosition = Vector3::Transform(worldTransform.position,parentWorldTransform.matrix.Invert());

                // Convert world orientation to parent-relative orientation
                Quaternion parentInverseOrientation;
                parentWorldTransform.orientation.Inverse(parentInverseOrientation);
                Quaternion relativeOrientation = parentInverseOrientation* worldTransform.orientation;

                // Update the local transform of the entity
                _transform[i].position = relativePosition;
                _transform[i].orientation = relativeOrientation;
            }
            else {
                // If there's no parent, the entity's transform is the world transform
                _transform[i] = worldTransform;
            }

            // 4. Update accordingly
            Matrix3 rotationMatrix;
            Matrix mat = XMMatrixRotationQuaternion(_transform[i].orientation);

            for (int col = 0; col < 3; ++col) {//Extract3X3
                for (int row = 0; row < 3; ++row) {
                    rotationMatrix[row * 3 + col] = mat.m[row][col];
                }
            }

            _rigidbody[i].m_inverseInertiaTensorWorld
                = (_rigidbody[i].m_inverseInertiaTensor * rotationMatrix) * rotationMatrix.Transpose();

            // 5. Clear
            _rigidbody[i].m_force = Vector3::Zero;
            _rigidbody[i].m_torque = Vector3::Zero;
        }
    }

    void IntegrateSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
    {
    }

    void IntegrateSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
    {
    }

    void IntegrateSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
    {
    }


    bool IntegrateSys::IsValid(const Vector3& vec) {
        return std::isfinite(vec.x) && std::isfinite(vec.y) && std::isfinite(vec.z);
    }

    bool IntegrateSys::IsValid(const Quaternion& quat) {
        return std::isfinite(quat.x) && std::isfinite(quat.y) && std::isfinite(quat.z) && std::isfinite(quat.w);
    }

}

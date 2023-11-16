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
#include "GravitySys.h"
#include "TransformSys.h"
#include "CollisionData.h"//collisionData
#include <iostream>

namespace Hostile {

    ADD_SYSTEM(IntegrateSys);

    void IntegrateSys::OnCreate(flecs::world& _world)
    {
        _world.system<Transform, MassProperties, Velocity, Force, InertiaTensor>("IntegrateSys")
            .rate(PHYSICS_TARGET_FPS_INV)
            .kind(IEngine::Get().GetIntegratePhase())
            .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("integrate phase place holder");
        //e.add<Force>();
    }

    void IntegrateSys::OnUpdate(flecs::iter& _it, Transform*_transform,MassProperties* _massProps,Velocity* _velocities,Force* forces, InertiaTensor* _inertiaTensor)
    {
        auto dt = _it.delta_time();

        for (int i = 0; i < _it.count(); i++) 
        {
            if (_massProps[i].inverseMass == 0.0f) {
                continue;
            }

            // 1. Linear Velocity
            Vector3 linearAcceleration = forces[i].force * _massProps[i].inverseMass;
            _velocities[i].linear += linearAcceleration * dt;
            _velocities[i].linear *= powf(.9f, dt);    //temp

            // 2. Angular Velocity
            Vector3 angularAcceleration = {_inertiaTensor[i].inverseInertiaTensorWorld * forces[i].torque};           //temp 
            _velocities[i].angular += angularAcceleration * dt;
            _velocities[i].angular *= powf(0.65f, dt);   //temp
            
            // 3. Calculate the new world position and orientation for the entity
            Transform worldTransform = TransformSys::GetWorldTransform(_it.entity(i));
            worldTransform.position += _velocities[i].linear * dt; // World position update

            // Check for NaN in position
            if (!IsValid(worldTransform.position)) {
                std::cerr << "Invalid world position for entity " << i << std::endl;
                continue;
            }

            Quaternion deltaRotation = Quaternion::Identity;
            if (_velocities[i].angular.LengthSquared() > FLT_EPSILON) {
                Vector3 angularVelocityNormalized = _velocities[i].angular;//
                angularVelocityNormalized.Normalize();
                float angularSpeed = _velocities[i].angular.Length();
                deltaRotation = Quaternion::CreateFromAxisAngle(angularVelocityNormalized, angularSpeed * dt);
            }
            worldTransform.orientation = deltaRotation * worldTransform.orientation; // World orientation update
            worldTransform.orientation.Normalize();

            // Check for NaN in orientation
            if (!IsValid(worldTransform.orientation)) {
                std::cerr << "Invalid world orientation for entity " << i << std::endl;
                continue;
            }

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

                if (!IsValid(_transform[i].position) || !IsValid(_transform[i].orientation)) {
                    std::cerr << "Invalid local transform for entity " << i << std::endl;
                    continue; // Skip this iteration or handle the error as needed
                }

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

            _inertiaTensor[i].inverseInertiaTensorWorld
                = (_inertiaTensor[i].inverseInertiaTensor * rotationMatrix) * rotationMatrix.Transpose();

            // 5. Clear
            forces[i].force = Vector3::Zero;
            forces[i].torque = Vector3::Zero;
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

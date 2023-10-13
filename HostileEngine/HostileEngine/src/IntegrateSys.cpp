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
#include "DetectCollisionSys.h"//collisionData
#include <iostream>

namespace Hostile {

    ADD_SYSTEM(IntegrateSys);

    void IntegrateSys::UpdateTransformMatrix(Transform& _transform)
    {
        _transform.matrix.m[0][0] =
            1.0f - 2.0f * (_transform.orientation.y * _transform.orientation.y + _transform.orientation.z * _transform.orientation.z);
        _transform.matrix.m[1][0] =
            2.0f * (_transform.orientation.x * _transform.orientation.y - _transform.orientation.w * _transform.orientation.z);
        _transform.matrix.m[2][0] =
            2.0f * (_transform.orientation.x * _transform.orientation.z + _transform.orientation.w * _transform.orientation.y);

        _transform.matrix.m[0][1] =
            2.0f * (_transform.orientation.x * _transform.orientation.y + _transform.orientation.w * _transform.orientation.z);
        _transform.matrix.m[1][1] =
            1.0f - 2.0f * (_transform.orientation.x * _transform.orientation.x + _transform.orientation.z * _transform.orientation.z);
        _transform.matrix.m[2][1] =
            2.0f * (_transform.orientation.y * _transform.orientation.z - _transform.orientation.w * _transform.orientation.x);

        _transform.matrix.m[0][2] =
            2.0f * (_transform.orientation.x * _transform.orientation.z - _transform.orientation.w * _transform.orientation.y);
        _transform.matrix.m[1][2] =
            2.0f * (_transform.orientation.y * _transform.orientation.z + _transform.orientation.w * _transform.orientation.x);
        _transform.matrix.m[2][2] =
            1.0f - 2.0f * (_transform.orientation.x * _transform.orientation.x + _transform.orientation.y * _transform.orientation.y);

        _transform.matrix.m[3][0] = _transform.position.x;
        _transform.matrix.m[3][1] = _transform.position.y;
        _transform.matrix.m[3][2] = _transform.position.z;
    }

    void IntegrateSys::OnCreate(flecs::world& _world)
    {
        _world.system<Transform, MassProperties, Velocity, Force, InertiaTensor>("IntegrateSys")
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
            _velocities[i].linear *= powf(.99f, dt);    //temp

            // 2. Angular Velocity
            Vector3 angularAcceleration = {_inertiaTensor->inverseInertiaTensorWorld*forces->torque};           //temp 
            _velocities[i].angular += angularAcceleration * dt;
            _velocities[i].angular *= powf(0.7f, dt);   //temp

            // 3. Pos, Orientation
            _transform[i].position += _velocities[i].linear * dt;
            {
                Vector3 scaledVelocity = _velocities[i].angular * dt;
                float angle = scaledVelocity.Length();
                Vector3 axis = (fabs(angle) < 1e-6f) ? Vector3(0, 1, 0) : scaledVelocity / angle;
                Quaternion deltaRotation = Quaternion::CreateFromAxisAngle(axis, angle);
                _transform[i].orientation = deltaRotation * _transform[i].orientation;
            }
            _transform[i].orientation.Normalize();

            // 4. Update accordingly
            //UpdateTransformMatrix(_transform[i]);
            Matrix3 rotationMatrix=Extract3x3Matrix(_transform[i].matrix); 
            _inertiaTensor->inverseInertiaTensorWorld
                = (_inertiaTensor->inverseInertiaTensor * rotationMatrix) * rotationMatrix.Transpose();

            // 5. Clear
            forces[i].force = Vector3::Zero;
            forces[i].torque = Vector3::Zero;
        }
        //for (int i = 0; i < _it.count(); i++)
        //{
        //    //UpdateTransformMatrix(_transform[i]);
        //}
    }

    void IntegrateSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components)
    {
    }

    void IntegrateSys::Read(flecs::entity& _object, nlohmann::json& _data)
    {
    }

    void IntegrateSys::GuiDisplay(flecs::entity& _entity)
    {
    }


}

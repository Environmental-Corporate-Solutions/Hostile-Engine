//------------------------------------------------------------------------------
//
// File Name:	PhysicsSimulationSys.h
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "directxtk/SimpleMath.h"
#include "ISystem.h"
#include "Matrix3.h"//temp

namespace Hostile
{
    using namespace DirectX::SimpleMath;
    class Matrix3;
    
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
        MassProperties(float mass=1.f)
        {
            assert(mass != 0.0f && "Mass can't be zero");
            inverseMass = 1.f / mass;
        }
    };

    struct Damping {
        float linearDamping=.95f;
        float angularDamping=.7f;
    };

    struct Gravity {
        Vector3 direction = { 0, -9.81f, 0 };  
    };

    struct RigidBody { //just a tag, might not be used
    };

    class GravitySys: public ISystem
    {
    private:

    public:
        virtual ~GravitySys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        static void OnUpdate(flecs::iter& it, Force* force, MassProperties* mass);
    };
}
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

using namespace DirectX::SimpleMath;
namespace Hostile
{
    struct RigidBody {
        Vector3 velocity;
        Vector3 angularVelocity;
        float inverseMass;
    };

    class GravitySys: public ISystem
    {
    private:

    public:
        virtual ~GravitySys() {}
        virtual void OnCreate(flecs::world& _world) override;
        static void OnUpdate(flecs::iter& _info, RigidBody* _bodies);
    };
}
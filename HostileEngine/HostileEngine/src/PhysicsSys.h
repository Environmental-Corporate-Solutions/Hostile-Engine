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

using namespace DirectX;
namespace Hostile
{
    struct RigidObject
    {
    };

    struct RigidBody {

    };

    class PhysicsSys: public ISystem
    {
    private:

    public:
        virtual ~PhysicsSys() {}
        virtual void OnCreate(flecs::world& _world) override;
        static void OnUpdate(flecs::iter& _info, RigidBody* bodies);
    };
}
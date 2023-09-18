//------------------------------------------------------------------------------
//
// File Name:	ItegrateSys.h
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
    struct Force {
        Vector3 force;
        Vector3 torque;
    };

    class IntegrateSys : public ISystem
    {
    private:

    public:
        virtual ~IntegrateSys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        static void OnUpdate(flecs::iter& _info);
    };
}
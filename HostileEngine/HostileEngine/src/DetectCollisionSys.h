//------------------------------------------------------------------------------
//
// File Name:	DetectCollisionSys.h
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
    struct Collider {
    };

    struct Constraint {
    };


    class DetectCollisionSys : public ISystem
    {
    private:

    public:
        virtual ~DetectCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override;
        static void OnUpdate(flecs::iter& it, Collider* colliders, Constraint* constraints);
    };
}
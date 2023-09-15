//------------------------------------------------------------------------------
//
// File Name:	ResolveCollisionSys.h
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
    struct CollisionData{};

    class ResolveCollisionSys : public ISystem
    {
    private:

    public:
        virtual ~ResolveCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override;
        static void OnUpdate(flecs::iter& _info, CollisionData* _collisionData);
    };
}
//------------------------------------------------------------------------------
//
// File Name:	DetecCollisionSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Engine.h"
#include "ResolveCollisionSys.h"

namespace Hostile {

    ADD_SYSTEM(ResolveCollisionSys);

    void ResolveCollisionSys::OnCreate(flecs::world& _world) {
        _world.system<CollisionData>("ResolveCollisionSys")
              .kind(IEngine::Get().GetResolveCollisionPhase())
              .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("resolve collision place holder");
        e.add<CollisionData>();

    }
    void ResolveCollisionSys::OnUpdate(flecs::iter& _info, CollisionData* _collisionData) {
    }
}

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
#include "DetectCollisionSys.h"

namespace Hostile {

    ADD_SYSTEM(DetectCollisionSys);
    
    void DetectCollisionSys::OnCreate(flecs::world& _world)
    {
        _world.system<Collider, Constraint>("DetectCollisionSys")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("detect collision place holder");
        e.add<Collider>();
        e.add<Constraint>();
    }

    void DetectCollisionSys::OnUpdate(flecs::iter& it, Collider* colliders, Constraint* constraints) {
    }

}

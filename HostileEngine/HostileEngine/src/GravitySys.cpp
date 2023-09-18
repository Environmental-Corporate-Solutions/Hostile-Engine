//------------------------------------------------------------------------------
//
// File Name:	PhysicsSimulationSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "GravitySys.h"
#include "Engine.h"

namespace Hostile {

    ADD_SYSTEM(GravitySys);
    void GravitySys::OnCreate(flecs::world& _world) {
        _world.system<RigidBody>("GravitySys")
              .kind(IEngine::Get().GetGravityPhase())
              .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("gravity phase place holder");
        e.add<RigidBody>();
    }


    void GravitySys::OnUpdate(flecs::iter& _info, RigidBody* _bodies) {
    }
}
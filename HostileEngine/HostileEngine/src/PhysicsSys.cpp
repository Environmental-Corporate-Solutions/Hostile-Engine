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
#include "PhysicsSys.h"
#include "Engine.h"

namespace Hostile {

    ADD_SYSTEM(PhysicsSys);
    void PhysicsSys::OnCreate(flecs::world& _world) {
        _world.system<RigidBody>("PhysicsSys")
              .kind(IEngine::Get().GetPhysicsPhase())
              .iter(OnUpdate);
        auto e = _world.entity();
        e.set_name("physics entity place holder");
        e.add<RigidBody>();
    }


    void PhysicsSys::OnUpdate(flecs::iter& _info, RigidBody* bodies) {
    }
}
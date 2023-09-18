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
#include "DetectCollisionSys.h"

namespace Hostile {

    ADD_SYSTEM(GravitySys);
    void GravitySys::OnCreate(flecs::world& _world) {
        _world.add<Gravity>();
        _world.system<Force, MassProperties>("GravitySys")
            .kind(IEngine::Get().GetGravityPhase())
            .iter(OnUpdate);

        
        //place holder entities
        {
            auto e1 = _world.entity();
            e1.set_name("Sphere RigidBody");
            e1.set<SphereCollider>(float{ 2.f });
            e1.add<Force>();
            e1.set<MassProperties>({ {2.f} });

            auto e2 = _world.entity();
            e2.set_name("Box RigidBody");
            e2.set<BoxCollider>(Vector3{ 2.f,2.f,2.f });
            e2.add<Force>();
            e2.set<MassProperties>({ {3.f} });

            auto e3 = _world.entity();
            e3.set_name("Plane Constraint");
            e3.add<Constraint>();
            //no mass component
        }
    }

    void GravitySys::OnUpdate(flecs::iter& _it, Force* force, MassProperties* mass) {
        const Vector3 GravitationalAcc = _it.world().get<Gravity>()->direction;
        const size_t Count = _it.count();
        for (int i = 0; i <Count; ++i) {
            force[i].force += GravitationalAcc * (1.f / mass[i].inverseMass);
        }
    }

}
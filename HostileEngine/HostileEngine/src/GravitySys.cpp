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
#include "TransformSys.h"
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
            //1. sphere
            const float Mass = 2.f;
            const float Rad = 2.f;
            Matrix3 inertiaTensor;
            inertiaTensor.SetDiagonal(0.4f * Mass);

            auto e1 = _world.entity();
            e1.set_name("Sphere");
            e1.set<SphereCollider>({Rad});
            e1.add<Velocity>();
            e1.add<Acceleration>();
            e1.add<Force>();
            e1.set<MassProperties>({Mass});
            e1.add<Matrix>();
            e1.set<Transform>({ {0.f,3.f,0.f},{Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) } });
            e1.set<InertiaTensor>({ {inertiaTensor.Inverse()}, {}});

            //2. box
            auto e2 = _world.entity();
            e2.set_name("Box");
            e2.set<BoxCollider>(Vector3{ 2.f,2.f,2.f });
            e2.add<Velocity>();
            e1.add<Acceleration>();
            e2.add<Force>();
            e2.set<MassProperties>({ {3.f} });
            e2.add<Matrix>();
            e2.add<Transform>();

            //3. plane
            auto e3 = _world.entity();
            e3.set_name("Plane");
            e3.add<Matrix>();
            e3.add<Constraint>();
            e3.set<Transform>({ {0.f,0.f,0.f},{Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) } });
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
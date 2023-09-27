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
#include "GraphicsSystem.h"//Mesh
#include "Rigidbody.h"//tag

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
            const float Mass = 5.f;
            const float Rad = 1.f;
            Matrix3 inertiaTensor;
            inertiaTensor.SetDiagonal(0.4f * Mass);

            auto e1 = _world.entity();
            e1.set_name("Sphere1").
                //set<SphereCollider>({ Rad }).
                set<BoxCollider>(Vector3{ Rad,Rad,Rad }).
                set<Velocity>({ {6,0,6},{-60,-55,-95} }).
                add<Acceleration>().
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ 
                    {-18.f,12.f,-18.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {1.f, 1.f, 1.f} }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Mesh>().set<Mesh>({ "Cube", 0 }).
                add<Rigidbody>();

			//e1 = _world.entity();
			//e1.set_name("Sphere2").
			//	set<SphereCollider>({ Rad }).
			//	set<Velocity>({ {0,0,0},{0,0,0} }).
			//	add<Acceleration>().
			//	add<Force>().
			//	set<MassProperties>({ Mass }).
			//	set<Transform>({ {-3.5f,15.f,-3.f},{Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) } }).
			//	set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
   //             add<Mesh>().set<Mesh>({ "Cube", 0 });

            ////2. box
            //auto e2 = _world.entity();
            //e2.set_name("Box");
            //e2.set<BoxCollider>(Vector3{ 2.f,2.f,2.f });
            //e2.add<Velocity>();
            //e1.add<Acceleration>();
            //e2.add<Force>();
            //e2.set<MassProperties>({ {3.f} });
            //e2.add<Matrix>();
            //e2.add<Transform>();

            //3. plane
			auto e3 = _world.entity();
			e3.set_name("Plane").
				add<Constraint>().
				set<Transform>({ {0.f,0.f,0.f},{Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) } });
            //TODO:: update mat
            //no mass component
        }
    }

    void GravitySys::OnUpdate(flecs::iter& _it, Force* force, MassProperties* mass) {
        //delete collisionData
        IEngine::Get().GetWorld().each<CollisionData>([](flecs::entity e, CollisionData& cd) {
            e.remove<CollisionData>();
            });

        const Vector3 GravitationalAcc = _it.world().get<Gravity>()->direction;
        const size_t Count = _it.count();
        for (int i = 0; i <Count; ++i) {
            force[i].force += GravitationalAcc * (1.f / mass[i].inverseMass);
        }
    }

}
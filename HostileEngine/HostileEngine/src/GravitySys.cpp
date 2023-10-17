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
//#include "GraphicsSystem.h"//Mesh
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
            const float Mass = 2.f;
            const float Scl = 1.f;

            Matrix3 inertiaTensor;
            inertiaTensor.SetDiagonal(Mass / 6.f);

            auto e1 = _world.entity("box1");
            e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,0,29.5},{0,0,0} }).
                set<Acceleration>({ { 0,0,0 }, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-2.f,1.2f,-50.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl, Scl} }).
                    set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();


            e1 = _world.entity("box2");
            e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,0,0},{0,0,0} }).
                set<Acceleration>({ { 0,0,0 }, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-2.f,6.2f,-2.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl, Scl} }).
                    set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();

            e1 = _world.entity("box3");
            e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,0,0},{0,0,0} }).
                set<Acceleration>({ {0,0,0}, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-2.3f,1.2f,-2.3f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl, Scl} }).
                    set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();

            inertiaTensor.SetDiagonal(Mass * 0.4);//sphere

            e1 = _world.entity("Sphere1");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {15,0,15},{0,0,0} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-21.5f,1.f,-20.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();

            e1 = _world.entity("Sphere2");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {-15,0,-15},{1,2,3} }).
                set<Acceleration>({{ 0,0,0 }, { 0,0,0 }}).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {24.5f,1.f,22.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();

            e1 = _world.entity("Sphere3");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {0,0,0},{10,10,10} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-2.5f,80.f,-2.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();

            e1 = _world.entity("Sphere4");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {0,0,0},{10,10,10} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-1.f,80.f,-1.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();


            //plane
			auto e3 = _world.entity("Plane");
			e3.add<Constraint>();
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

    void GravitySys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components)
    {
    }

    void GravitySys::Read(flecs::entity& _object, nlohmann::json& _data)
    {
    }

    void GravitySys::GuiDisplay(flecs::entity& _entity)
    {
    }

}
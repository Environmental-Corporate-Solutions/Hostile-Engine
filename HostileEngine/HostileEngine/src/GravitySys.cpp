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
    /**
     * GravitySys::OnCreate
     * @brief Gravity Phases Init Sequence
     *
     * To maintain a consistent delta time ('dt') across all physics phases:
     * - Only the initial phase, gravity, is set with a fixed rate.
     * - Subsequent phases execute in sequence based on their dependencies.
     *
     * Setting individual rates for each phase could lead to unintended frame skips
     * or delays due to their interdependencies. Thus, only the gravity phase's rate
     * is explicitly set to ensure uniformity and predictability in execution.
     */
    ADD_SYSTEM(GravitySys);
    void GravitySys::OnCreate(flecs::world& _world) {
        _world.add<Gravity>();
        _world.system<Force, MassProperties>("GravitySys")
            .rate(PHYSICS_TARGET_FPS_INV)
            .kind(IEngine::Get().GetGravityPhase())
            .iter(OnUpdate);
        
        //place holder entities
        {
            //1. sphere
            const float Mass = 2.f;
            const float Scl = 1.f;
            const float Scl2 = 2.f;

            Matrix3 inertiaTensor;
            inertiaTensor.SetDiagonal(Mass / 6.f);

            auto e1 = _world.entity("box1");
            e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,13.7,-30},{0,1,0.1} }).
                set<Acceleration>({ {0,0,0}, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-2.2f,1.f,85.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl2, Scl} }).
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
                    {Scl2, Scl, Scl2} }).
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
                    {Scl, Scl2, Scl2} }).
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
                    {Scl2,Scl2,Scl2}
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
                set<Transform>({ {-2.5f,55.f,-2.5f},
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
                set<Transform>({ {-1.f,45.f,-1.5f},
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

        const Vector3 GravitationalAcc = _it.world().get<Gravity>()->direction;
        const size_t Count = _it.count();
        for (int i = 0; i <Count; ++i) {
            force[i].force += GravitationalAcc * (1.f / mass[i].inverseMass);
        }
    }

    void GravitySys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
    {
    }

    void GravitySys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
    {
    }

    void GravitySys::GuiDisplay(flecs::entity& _entity, const std::string& type)
    {
    }

}
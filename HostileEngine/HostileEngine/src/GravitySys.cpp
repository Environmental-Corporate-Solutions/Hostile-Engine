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
#include "PhysicsProperties.h"
#include "TransformSys.h"

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

        REGISTER_TO_SERIALIZER(Gravity, this);
        REGISTER_TO_DESERIALIZER(Gravity, this);
        IEngine::Get().GetGUI().RegisterComponent(
            "Gravity",
            std::bind(&GravitySys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
            [this](flecs::entity& _entity) { _entity.add<Gravity>(); });


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
                set<Velocity>({ {0,0,0},{0,1,0.1} }).
                set<Acceleration>({ {0,0,0}, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-1.2f,1.f,1.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl2, Scl} }).
                    set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "box1" });

            e1 = _world.entity("box2");
                e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,0,0},{0,0,0} }).
                set<Acceleration>({ { 0,0,0 }, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {-1.f,7.2f,0.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl2, Scl, Scl2} }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "box2" });

            e1 = _world.entity("box3");
                e1.add<BoxCollider>().//tag
                set<Velocity>({ {0,0,0},{0,0,0} }).
                set<Acceleration>({ {0,0,0}, {0,0,0} }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({
                    {0.5f,1.2f,0.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl, Scl2, Scl2} }).
                    set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "box3" });

            inertiaTensor.SetDiagonal(Mass * 0.4);//sphere

            e1 = _world.entity("Sphere1");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {3,0,3},{0,0,0} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-5.5f,1.f,-5.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl2,Scl2,Scl2}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "Shere1" });

            e1 = _world.entity("Sphere2");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {-15,0,-15},{1,2,3} }).
                set<Acceleration>({{ 0,0,0 }, { 0,0,0 }}).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {24.5f,2.f,22.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "Shere2" });

            e1 = _world.entity("Sphere3");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {3,0,6},{10,10,10} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-4.5f,1.5f,-9.f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "Shere3" });

            e1 = _world.entity("Sphere4");
            e1.add<SphereCollider>().//tag
                set<Velocity>({ {0.2,0,0.2},{10,10,10} }).
                set<Acceleration>({ { 0,0,0 }, { 0,0,0 } }).
                add<Force>().
                set<MassProperties>({ Mass }).
                set<Transform>({ {-1.f,15.f,-1.5f},
                    {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
                    {Scl,Scl,Scl}
                    }).
                set<InertiaTensor>({ {inertiaTensor.Inverse()}, {} }).
                add<Rigidbody>();
            e1.set<ObjectName>({ "Shere4" });

            float tiltAngleX = -DirectX::XM_PI / 20;
            float tiltAngleZ = -DirectX::XM_PI / 20; 
            DirectX::SimpleMath::Quaternion tiltQuaternionX = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, tiltAngleX);
            DirectX::SimpleMath::Quaternion tiltQuaternionZ = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, tiltAngleZ);
            DirectX::SimpleMath::Quaternion combinedTilt = tiltQuaternionX * tiltQuaternionZ;

            //plane
			auto e3 = _world.entity("Plane");
            e3.add<Constraint>().
                set<Transform>({ {0.f,-1.5f,0.f},
                    {combinedTilt},                                            // tilted plane
                    //{Quaternion::CreateFromAxisAngle(Vector3::UnitZ, 0.f) }, // non-tilted plane
                    {100.f,1.f,100.f}
                    }).set<ObjectName>({"Plane"});

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
        if (type == "Gravity")
        {
            const Gravity* gravity = _entity.get<Gravity>();
            if (gravity)
            {
                nlohmann::json obj = nlohmann::json::object();
                obj["Type"] = "Gravity";
                obj["Direction"] = { gravity->direction.x, gravity->direction.y, gravity->direction.z };
                _components.push_back(obj);
            }
        }
    }

    void GravitySys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
    {
        if (type == "Gravity")
        {
            Vector3 direction;
            direction.x = _data["Direction"][0];
            direction.y = _data["Direction"][1];
            direction.z = _data["Direction"][2];
            _object.set<Gravity>({ direction });
        }
    }

    void GravitySys::GuiDisplay(flecs::entity& _entity, const std::string& type)
    {
        if (type == "Gravity")
        {
            if (_entity.has<Gravity>())
            {
                Gravity* gravity = _entity.get_mut<Gravity>();
                if (ImGui::TreeNodeEx("Gravity", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::DragFloat3("Direction", &gravity->direction.x, 0.1f);
                    ImGui::TreePop();
                }
            }
        }
    }

}
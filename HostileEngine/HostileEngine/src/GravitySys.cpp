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
        _world.system<Rigidbody>("GravitySys")
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
   //     {
   //         Scene& scene = *IEngine::Get().GetCurrentScene();
   //         //1. sphere
   //         const float Mass = 2.f;
   //         const float Scl = 1.f;
   //         const float Scl2 = 2.f;

   //         Matrix3 inertiaTensor;
   //         inertiaTensor.SetDiagonal(Mass / 6.f);

   //         auto e1 = _world.entity("box1");
   //         e1.add<BoxCollider>().//tag
   //             set<Transform>({
   //                 {-1.2f,1.f,-1.5f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl2, Scl2, Scl} }).
   //             set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                 {0,0,0},//linear velocity
   //                                 {0,0,0},//linear acc
   //                                 {0,0,0},//angular vel
   //                                 {0,0,0},//angular acc
   //                                 {0,0,0},//force
   //                                 {0,0,0},//torque
   //                                 Mass
   //                                 //,linearDamping
   //                                 //,angularDamping
   //                                 //,useGraivy
   //                             }).
   //             set<ObjectName>({ "box1" });
   //         scene.Add(e1);

   //         e1 = _world.entity("box2");
   //             e1.add<BoxCollider>().//tag
   //             set<Transform>({
   //                 {-1.f,7.2f,0.5f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl2, Scl, Scl2} }).
   //                 set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                     {0,0,0},//linear velocity
   //                                     {0,0,0},//linear acc
   //                                     {0,0,0},//angular vel
   //                                     {0,0,0},//angular acc
   //                                     {0,0,0},//force
   //                                     {0,0,0},//torque
   //                                     Mass
   //                                 //,linearDamping
   //                                 //,angularDamping
   //                                 //,useGraivy
   //                                 }).
   //                 set<ObjectName>({ "box2" });
   //             scene.Add(e1);

   //         e1 = _world.entity("box3");
   //             e1.add<BoxCollider>().//tag
   //             set<Transform>({
   //                 {0.5f,1.2f,0.5f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl, Scl2, Scl2} }).
   //                 set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                     {0,0,0},//linear velocity
   //                                     {0,0,0},//linear acc
   //                                     {0,0,0},//angular vel
   //                                     {0,0,0},//angular acc
   //                                     {0,0,0},//force
   //                                     {0,0,0},//torque
   //                                     Mass
   //                                 //,linearDamping
   //                                 //,angularDamping
   //                                 //,useGraivy
   //                                 }).
   //                 set<ObjectName>({ "box3" });
   //             scene.Add(e1);

   //             float tiltAngleX = -DirectX::XM_PI / 20;
   //             float tiltAngleZ = -DirectX::XM_PI / 20;

   //             DirectX::SimpleMath::Quaternion tiltQuaternionX = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, tiltAngleX);
   //             DirectX::SimpleMath::Quaternion tiltQuaternionZ = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, tiltAngleZ);
   //             DirectX::SimpleMath::Quaternion combinedTilt = tiltQuaternionX * tiltQuaternionZ;

   //             const float StarMass = 1000.f;
   //             Matrix3 pivotInertiaTensor;
   //             inertiaTensor.SetDiagonal(StarMass / 6.f);

   //             auto pivot = _world.entity("pivot");
   //             pivot.add<BoxCollider>().//tag
   //                 set<Transform>({
   //                     {7.5f,2.5f,-18.5f},
   //                     {combinedTilt },
   //                     {Scl, Scl, Scl2} }).
   //                     set<Rigidbody>({ {pivotInertiaTensor.Inverse()},//inverse inertianTensor
   //                                         {0,0,0},//linear velocity
   //                                         {0,0,0},//linear acc
   //                                         {0,0,0},//angular vel
   //                                         {0,0,0},//angular acc
   //                                         {0,0,0},//force
   //                                         {0,200,0},//torque
   //                                         StarMass,
   //                                         0.9f,//linearDamping
   //                                         1.f,//,angularDamping
   //                                         false//,useGraivy
   //                         }).
   //                 set<ObjectName>({ "pivot" });
   //             scene.Add(pivot);

   //             const float PlanetMass = 500.f;
   //             Matrix3 planetInertiaTensor;
   //             inertiaTensor.SetDiagonal(PlanetMass / 6.f);

   //             auto planet = _world.entity("planet");
   //             planet.add<SphereCollider>().//tag
   //                 set<Transform>({
   //                     {0.f,-5.5f,6.5f},
   //                     {combinedTilt},
   //                     {Scl2, Scl2, Scl} }).
   //                     set<Rigidbody>({ {planetInertiaTensor.Inverse()},//inverse inertianTensor
   //                                         {0,0,0},//linear velocity
   //                                         {0,0,0},//linear acc
   //                                         {0,0,0},//angular vel
   //                                         {0,0,0},//angular acc
   //                                         {0,0,0},//force
   //                                         {0,30,0},//torque
   //                                         PlanetMass,
   //                                         0.9f,//linearDamping
   //                                         1.f,//,angularDamping
   //                                         true//,useGraivy
   //                         }).
   //                 set<ObjectName>({ "planet" });
   //             scene.Add(planet);

   //             planet.child_of(pivot);

   //         inertiaTensor.SetDiagonal(Mass * 0.4);//sphere

   //         e1 = _world.entity("Sphere1");
   //         e1.add<SphereCollider>().//tag
   //             set<Transform>({ {-5.5f,1.f,-5.f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl2,Scl2,Scl2}
   //                 }).
   //             set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                 {3,0,3},//linear velocity
   //                                 {0,0,0},//linear acc
   //                                 {0,0,0},//angular vel
   //                                 {0,0,0},//angular acc
   //                                 {0,0,0},//force
   //                                 {0,0,0},//torque
   //                                 Mass
   //                             //,drag
   //                             //linearDamping
   //                             //,angularDamping
   //                 }).
   //             set<ObjectName>({ "Shere1" });
   //         scene.Add(e1);

   //         e1 = _world.entity("Sphere2");
   //         e1.add<SphereCollider>().//tag
   //             set<Transform>({ {24.5f,2.f,22.5f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl,Scl,Scl}
   //                 }).
   //             set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                 {-15,0,-15},//linear velocity
   //                                 {0,0,0},//linear acc
   //                                 {0,0,0},//angular vel
   //                                 {0,0,0},//angular acc
   //                                 {0,0,0},//force
   //                                 {0,0,0},//torque
   //                                 Mass
   //                             //,linearDamping
   //                             //,angularDamping
   //                             //,useGraivy
   //                             }).
   //             set<ObjectName>({ "Shere2" });
   //         scene.Add(e1);

   //         e1 = _world.entity("Sphere3");
   //         e1.add<SphereCollider>().//tag
   //             set<Transform>({ {-4.5f,1.5f,-9.f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl,Scl,Scl}
   //                 }).
   //             set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                 {3,0,6},//linear velocity
   //                                 {0,0,0},//linear acc
   //                                 {0,0,0},//angular vel
   //                                 {0,0,0},//angular acc
   //                                 {0,0,0},//force
   //                                 {0,0,0},//torque
   //                                 Mass
   //                                 //,linearDamping
   //                                 //,angularDamping
   //                                 //,useGraivy
   //                                 }).
   //             set<ObjectName>({ "Shere3" });
   //         scene.Add(e1);

   //         e1 = _world.entity("Sphere4");
   //         e1.add<SphereCollider>().//tag
   //             set<Transform>({ {-1.f,15.f,-1.5f},
   //                 {Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
   //                 {Scl,Scl,Scl}
   //                 }).
   //             set<Rigidbody>({ {inertiaTensor.Inverse()},//inverse inertianTensor
   //                                 {0,0,0},//linear velocity
   //                                 {0,0,0},//linear acc
   //                                 {0,0,0},//angular vel
   //                                 {0,0,0},//angular acc
   //                                 {0,0,0},//force
   //                                 {0,0,0},//torque
   //                                 Mass
   //                 //,linearDamping
   //                 //,angularDamping
   //                 //,useGraivy
   //                 }).
   //             set<ObjectName>({ "Shere4" });
   //         scene.Add(e1);

   //         //plane
			//auto e3 = _world.entity("Plane");
   //         e3.add<PlaneCollider>().
   //             set<Transform>({ {0.f,-1.5f,0.f},
   //                 {combinedTilt},                                            // tilted plane
   //                 //{Quaternion::CreateFromAxisAngle(Vector3::UnitZ, 0.f) }, // non-tilted plane
   //                 {100.f,1.f,100.f}
   //                 }).set<ObjectName>({"Plane"});
   //                 scene.Add(e3);
   //     }
    }

    void GravitySys::OnUpdate(flecs::iter& _it, Rigidbody* _rigidbody) 
    {

        const Vector3 GravitationalAcc = _it.world().get<Gravity>()->direction;
        const size_t Count = _it.count();
        for (int i = 0; i <Count; ++i) 
        {
            if (_rigidbody[i].m_useGravity == true)
            {
                _rigidbody[i].m_force+=GravitationalAcc * (1.f / _rigidbody[i].m_inverseMass);
            }
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
        //if (type == "Gravity")
        //{
        //    if (_entity.has<Gravity>())
        //    {
        //        Gravity* gravity = _entity.get_mut<Gravity>();
        //        if (ImGui::TreeNodeEx("Gravity", ImGuiTreeNodeFlags_DefaultOpen))
        //        {
        //            ImGui::DragFloat3("Direction", &gravity->direction.x, 0.1f);
        //            ImGui::TreePop();
        //        }
        //    }
        //}
    }

}
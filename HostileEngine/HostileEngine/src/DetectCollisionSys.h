//------------------------------------------------------------------------------
//
// File Name:	DetectCollisionSys.h
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once

#include "directxtk12/SimpleMath.h"
#include "ISystem.h"
#include <utility>//std::pair

using namespace DirectX::SimpleMath;

namespace Hostile
{
    class Transform;

    struct SphereCollider {
        //float radius;
        //SphereCollider(float r = 1.f) :radius {r}
        //{}
    };

    struct BoxCollider {
        //Vector3 extents;
        //BoxCollider(const Vector3& v = Vector3{1.f,1.f,1.f}) : extents(v) 
        //{}
    };

    struct Constraint { //plane (for now)
        Vector3 normal;
        float offset;
        Constraint(const Vector3& n = Vector3{ 0.f,1.f,0.f }, float Offset = 0.f) :normal{ n }, offset{ Offset }
        {}
    };

    struct CollisionData {
        flecs::entity entity1;
        flecs::entity entity2;  // the other entity involved in the collision
        Vector3 collisionNormal;
        std::pair<Vector3,Vector3> contactPoints;
        float penetrationDepth=0.f;
        float restitution=0.f;
        float friction=0.f;
        float accumulatedNormalImpulse=0.f; //perpendicular to the collision surface, (frictions are parallel)
    };


    class DetectCollisionSys : public ISystem
    {
    private:
        static bool IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd);
        static bool IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b);
        static bool IsColliding(const Transform& _tSphere, const Constraint& _c, float& distance);
        static bool IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2);
        static bool IsColliding(const Transform& _tBox, const BoxCollider& _b, const Constraint& _c);

    public:
        virtual ~DetectCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;

        static void TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres);
        static void TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components) override;
    };
}
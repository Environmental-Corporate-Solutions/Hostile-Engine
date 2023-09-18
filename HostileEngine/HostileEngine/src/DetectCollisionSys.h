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

#include "directxtk/SimpleMath.h"
#include "ISystem.h"

using namespace DirectX::SimpleMath;

namespace Hostile
{
    struct SphereCollider {
        float radius;
        SphereCollider(float r = 1.f) :radius {r}
        {}
    };

    struct BoxCollider {
        Vector3 extents;
        BoxCollider(const Vector3& v = Vector3{1.f,1.f,1.f}) : extents(v) 
        {}
    };

    struct Constraint { //plane (for now)
        Vector3 normal;
        float offset;
        Constraint(const Vector3& n = Vector3{ 0.f,1.f,0.f }, float Offset = 0.f) :normal{ n }, offset{ Offset }
        {}
    };

    class DetectCollisionSys : public ISystem
    {
    private:

    public:
        virtual ~DetectCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;

        static bool IsColliding(const SphereCollider& s1, const SphereCollider& s2);
        static bool IsColliding(const SphereCollider& s, const BoxCollider& b);
        static bool IsColliding(const SphereCollider& s, const Constraint& c);
        static bool IsColliding(const BoxCollider& b1, const BoxCollider& b2);
        static bool IsColliding(const BoxCollider& b, const Constraint& c);


        static void TestSphereCollision(flecs::iter& it, SphereCollider* spheres);
        static void TestBoxCollision(flecs::iter& it, BoxCollider* boxes);
    };
}
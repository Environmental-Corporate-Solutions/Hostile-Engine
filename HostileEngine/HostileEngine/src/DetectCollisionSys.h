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
    };

    struct BoxCollider {
        Vector3 extents;
    };

    struct PlaneCollider {
        Vector3 normal;
        float offset;
    };

    struct Constraint {
    };

    class DetectCollisionSys : public ISystem
    {
    private:

    public:
        virtual ~DetectCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override;

        static void OnUpdateSphereConstraint(flecs::iter& it, SphereCollider* _colliders, Constraint* _constraints);
        static void OnUpdateSphereSphere(flecs::iter& it, SphereCollider* _colliders, SphereCollider* _constraints);
        static void OnUpdateBoxConstraint(flecs::iter& it, BoxCollider* _colliders, Constraint* _constraints);
        static void OnUpdateBoxBox(flecs::iter& it, BoxCollider* _colliders, BoxCollider* _constraints);
        static void OnUpdateBoxSphere(flecs::iter& it, BoxCollider* _colliders, SphereCollider* _constraints);        
    };
}
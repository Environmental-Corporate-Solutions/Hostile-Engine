//------------------------------------------------------------------------------
//
// File Name:	DetecCollisionSys.cpp
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Engine.h"
#include "DetectCollisionSys.h"

namespace Hostile {

    ADD_SYSTEM(DetectCollisionSys);
    
    void DetectCollisionSys::OnCreate(flecs::world& _world)
    {
        // Sphere interactions with constraints
        _world.system<SphereCollider, Constraint>("OnUpdateSphereConstraint")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdateSphereConstraint);

        _world.system<SphereCollider, SphereCollider>("OnUpdateSphereSphere")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdateSphereSphere);

        _world.system<BoxCollider, Constraint>("OnUpdateBoxConstraint")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdateBoxConstraint);

        _world.system<BoxCollider, BoxCollider>("OnUpdateBoxBox")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdateBoxBox);

        _world.system<BoxCollider, SphereCollider>("OnUpdateBoxSphere")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(OnUpdateBoxSphere);

        // 1. Entity with a SphereCollider and a Constraint
        auto e = _world.entity();
        e.set_name("Entity with SphereCollider and Constraint");
        e.add<SphereCollider>();
        e.add<Constraint>();

        // 2. Entity with two SphereColliders
        e = _world.entity();
        e.set_name("Entity with two SphereColliders");
        e.add<SphereCollider>();
        e.add<SphereCollider>();

        // 3. Entity with both a BoxCollider and a Constraint
        e = _world.entity();
        e.set_name("Entity with BoxCollider and Constraint");
        e.add<BoxCollider>();
        e.add<Constraint>();

        // 4. Entity with both two BoxColliders
        e = _world.entity();
        e.set_name("Entity with two BoxColliders");
        e.add<BoxCollider>();
        e.add<BoxCollider>();

        // 5. Entity with both a BoxCollider and a SphereCollidert
        e = _world.entity();
        e.set_name("Entity with BoxCollider and SphereCollider");
        e.add<BoxCollider>();
        e.add<SphereCollider>();
    }

    void DetectCollisionSys::OnUpdateSphereConstraint(flecs::iter& it, SphereCollider* _colliders, Constraint* _constraints)
    {
    }

    void DetectCollisionSys::OnUpdateSphereSphere(flecs::iter& it, SphereCollider* _colliders, SphereCollider* _constraints)
    {
    }

    void DetectCollisionSys::OnUpdateBoxConstraint(flecs::iter& it, BoxCollider* _colliders, Constraint* _constraints)
    {
    }

    void DetectCollisionSys::OnUpdateBoxBox(flecs::iter& it, BoxCollider* _colliders, BoxCollider* _constraints)
    {
    }

    void DetectCollisionSys::OnUpdateBoxSphere(flecs::iter& it, BoxCollider* _colliders, SphereCollider* _constraints)
    {
    }
}

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
        _world.system<SphereCollider>("TestSphereCollision")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(TestSphereCollision);

        _world.system<BoxCollider>("TestBoxCollision")
            .kind(IEngine::Get().GetDetectCollisionPhase())
            .iter(TestBoxCollision);
    }
    bool DetectCollisionSys::IsColliding(const SphereCollider& s1, const SphereCollider& s2)
    {
        return true;
    }
    bool DetectCollisionSys::IsColliding(const SphereCollider& s, const BoxCollider& b)
    {
        return true;
    }
    bool DetectCollisionSys::IsColliding(const SphereCollider& s, const Constraint& c)
    {
        return true;
    }
    bool DetectCollisionSys::IsColliding(const BoxCollider& b1, const BoxCollider& b2)
    {
        return true;
    }
    bool DetectCollisionSys::IsColliding(const BoxCollider& b, const Constraint& c)
    {
        return true;
    }
    void DetectCollisionSys::TestSphereCollision(flecs::iter& it, SphereCollider* spheres) {
        // Sphere vs. Sphere
        for (int i = 0; i < it.count(); ++i) {
            for (int j = i + 1; j < it.count(); ++j) {
                if (IsColliding(spheres[i], spheres[j])) {
                    //TODO
                }
            }
        }

        // Sphere vs. Box
        it.world().each<BoxCollider>([&spheres, &it](flecs::entity e, BoxCollider& box) {
            for (int k = 0; k < it.count(); ++k) {
                if (IsColliding(spheres[k], box)) {
                    //TODO
                }
            }
            });

        // Sphere vs. Constraint
        it.world().each<Constraint>([&spheres, &it](flecs::entity e, Constraint& constraint) {
            for (int k = 0; k < it.count(); ++k) {
                if (IsColliding(spheres[k], constraint)) {
                    //TODO
                }
            }
            });
    }

    void DetectCollisionSys::TestBoxCollision(flecs::iter& it, BoxCollider* boxes) {
        // Box vs. Box
        for (int i = 0; i < it.count(); ++i) {
            for (int j = i + 1; j < it.count(); ++j) {
                if (IsColliding(boxes[i], boxes[j])) {
                    //TODO
                }
            }
        }

        // Box vs. Constraint
        it.world().each<Constraint>([&boxes, &it](flecs::entity e, Constraint& constraint) {
            for (int k = 0; k < it.count(); ++k) {
                if (IsColliding(boxes[k], constraint)) {
                    //TODO
                }
            }
            });
    }
}

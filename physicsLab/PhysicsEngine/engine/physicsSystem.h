#pragma once

#include "body.h"
#include "collisionManager.h"
#include "engine/contact.h"
#include "simulator/object.h"
#include <memory>//std::unique_ptr
#include <vector>
#include <unordered_map>

namespace physics
{
    struct PhysicsSystem
    {
        using Manifolds = std::vector<physics::CollisionData>;

        static float m_gravity;

    //private:

        std::vector<std::unique_ptr<RigidObject>> m_objects;
        std::vector<std::unique_ptr<Constraint>> m_constraints;

        CollisionSystem collisionSystem;

    //public:
        PhysicsSystem();

        void Simulate(double dt);

        void AddBoxRigidBody(Vector3 pos, RigidObject* obj);
        void AddBoxCollider(RigidBody*, RigidObject* obj);
    };
}
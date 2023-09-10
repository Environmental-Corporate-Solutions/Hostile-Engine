#pragma once

#include "body.h"
#include <vector>
#include <utility>//std::pair

namespace physics
{
    //this struct refers to a specific space that contains all the possible initial conditions and final outcomes of a collision between objects
    struct CollisionData
    {
        RigidBody* m_bodies[2];
        Vector3 m_collisionNormal; //dir : body0 <--- body1
        float m_penetrationDepth;
        float m_restitution;
        float m_friction;
        float m_accumulatedNormalImpulse; //perpendicular to the collision surface, (frictions are parallel)
        std::vector<std::pair<Vector3,Vector3>> m_contactPoints;

        CollisionData() :m_collisionNormal{}, m_penetrationDepth{}, m_restitution{}, m_friction{}, m_accumulatedNormalImpulse{} {
            m_bodies[0] = m_bodies[1] = nullptr;
        }
    };
} 
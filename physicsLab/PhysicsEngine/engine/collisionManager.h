#pragma once

#include "collider.h"
#include "simulator/object.h"
#include <memory>//std::unique_ptr
#include <vector>
#include <unordered_map>

namespace physics
{
    struct CollisionSystem
    {        
        float m_friction;
        float m_objectRestitution;
        float m_groundRestitution;
        float m_penetrationTolerance;
        float m_closingSpeedTolerance;
        int m_solverIterations;

        std::vector<physics::CollisionData> m_contacts;

    //public:
        CollisionSystem()
            : m_friction(0.65f), m_objectRestitution(0.48f), m_groundRestitution(0.18f),
            m_solverIterations(25), m_penetrationTolerance(0.0005f), m_closingSpeedTolerance(0.005f) {}
    
        void DetectCollision(const std::vector<std::unique_ptr<RigidObject>>& objects, const std::vector<std::unique_ptr<Constraint>>& constraints);
        void ResolveCollision(double deltaTime);
    
    private:
        //(1)RigidBodies
        bool FindCollisionFeatures(const Collider*,const Collider*);
        bool FindCollisionFeatures(const SphereCollider*,const SphereCollider*,bool isForBroadPhaseTest=false);

        //(2)Constraints
        bool FindCollisionFeatures(const Collider*,const Constraint*);
        bool FindCollisionFeatures(const SphereCollider*,const Plane*);    

    private:
        void SequentialImpulse(CollisionData contact, double dt, int j);
        void ApplyImpulses(CollisionData& contact, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction);
        void ApplyFrictionImpulses(CollisionData& contact, const Vector3& r1, const Vector3& r2);
        float ComputeTangentialImpulses(CollisionData& contact, const Vector3& r1, const Vector3& r2, const Vector3& tangent);
    };
}
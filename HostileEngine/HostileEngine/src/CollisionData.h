#pragma once
namespace Hostile {
    struct CollisionData {
        flecs::entity entity1; //TODO:: to id
        flecs::entity entity2;  // the other entity involved in the collision
        Vector3 collisionNormal;
        std::pair<Vector3, Vector3> contactPoints;
        float penetrationDepth = 0.f;
        float restitution = 0.f;
        float friction = 0.f;
        float accumulatedNormalImpulse = 0.f; //perpendicular to the collision surface, (frictions are parallel)
    };
    struct TriggerData
    {
        flecs::id_t triggerId;
        flecs::id_t nonTriggerId;
        TriggerData(flecs::id_t _triggerId, flecs::id_t _nonTriggerId)
            :triggerId{ _triggerId }, nonTriggerId{ _nonTriggerId }
        {}
    };
}
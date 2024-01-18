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
    struct CollisionTriggerEvent {
        enum class Type { Begin, Persist, End } type;
        flecs::id_t entityId1;
        flecs::id_t entityId2;

        CollisionTriggerEvent(Type _type, flecs::id_t _id1, flecs::id_t _id2)
            : type(_type), entityId1(_id1), entityId2(_id2) {}
    };
}
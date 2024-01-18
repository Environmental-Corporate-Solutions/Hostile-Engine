#pragma once
#include <flecs.h>
#include <utility>//pair
#include "directxtk12/SimpleMath.h"

namespace Hostile {
    using namespace DirectX::SimpleMath;
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
    struct CollisionEvent {
        enum class Type { Unkown,Begin, Persist, End } m_type;
        enum class Category { Unkown, Collision, Trigger } m_category;
        flecs::id_t entityId1;
        flecs::id_t entityId2;
        CollisionEvent(Type _type = Type::Unkown, Category _category = Category::Unkown, flecs::id_t _id1 = 0, flecs::id_t _id2=0)
            : m_type(_type), m_category(_category), entityId1(_id1), entityId2(_id2) {}
    };
}
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

#include "directxtk12/SimpleMath.h"
#include "ISystem.h"
#include "CollisionData.h"
#include "Matrix3.h"
#include "PhysicsProperties.h"
#include <vector>
#include <utility>//std::pair
#include <optional>
#include <unordered_set>
#include <mutex>

using namespace DirectX::SimpleMath;

namespace Hostile
{
    struct Transform;

    class CollisionSys : public ISystem
    {        
        //detect
        static bool IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd);
        static bool IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b);
        static bool IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _constraintOffset, float& distance);
        static bool IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2);
        static bool IsColliding(const Transform& _tBox, const BoxCollider& _b, const PlaneCollider& _c);
		static float CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& colliderScale1, const Vector3& colliderScale2, const Vector3& colliderOffset1, const Vector3& colliderOffset2, const Vector3& axis);
        static void CalcOBBsContactPoints(const Transform& t1, const Transform& t2, CollisionData& newContact, int minPenetrationAxisIdx);
		static Vector3 GetLocalContactVertex(Vector3 collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const cmp);
        static Vector3 GetAxis(const Quaternion& orientation, int index);
        //events
        static void AddTriggerState(flecs::id_t _triggerId, flecs::id_t _nonTriggerId);
        static void AddCollisionState(flecs::id_t _entity1Id, flecs::id_t _entity2Id);
        static void UpdateCollisionEvents();

        //resolve
        static void ApplyImpulses(flecs::entity _e1, flecs::entity _e2, float _jacobianImpulse, const Vector3& _r1, const Vector3& _r2, const Vector3& _direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static float ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2, const CollisionData& _collision);
        static void ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static void ResolveCollisions();
        static void TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres);
        static void TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes);
        static void Integrate(flecs::iter& _it, Transform* _transform, Rigidbody* _rigidbody);

        //mutex - working on it
        static void AddCollisionData(const CollisionData& data);
        static void ClearCollisionData();

        static constexpr float PLANE_OFFSET = 0.5f;
        static constexpr Vector3 UP_VECTOR{ 0, 1.f, 0 };//to convert quaternions to Vector3s

    public:
        using CollisionEventKey = std::tuple<flecs::id_t, flecs::id_t, CollisionEvent::Category>;
        struct EventKeyHash {
            std::size_t operator()(const CollisionEventKey& key) const {
                auto hash1 = std::hash<flecs::id_t>{}(std::get<0>(key));
                auto hash2 = std::hash<flecs::id_t>{}(std::get<1>(key));

                //auto hash3 = std::hash<int>{}(static_cast<int>(std::get<2>(key))); // hash for category
                //return hash1 ^ (hash2 << 1) ^ (hash3 << 2);

                return hash1 ^ hash2; //simple hashing, assuming hash1 & hash2 are always different.
            }
        };

        CollisionSys() 
        {
            m_collisionData.reserve(300);
        }
        virtual ~CollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;

        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
 


        //actual collisions info
        static std::vector<CollisionData> m_collisionData;

        // Map each entity to a vector of CollisionEvent objects.
        // This structure allows tracking multiple collision events per entity.
        static std::unordered_map<flecs::id_t, std::vector<CollisionEvent>> m_collisionEvents;

        //each frame starts with a clean slate, and collisions are processed based on current dynamics. 
        //this could inefficient but this helps prevent stale or incorrect collision handling. but might have to update this later
        static std::unordered_set<CollisionEventKey, EventKeyHash> m_currentCollisionEvents;
        static std::unordered_set<CollisionEventKey, EventKeyHash> m_previousCollisionEvents;

        static constexpr int SOLVER_ITERS = 3;

        static std::mutex collisionDataMutex;

        static float m_accumulatedTime;
    };
}
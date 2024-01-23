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
#include <set>

using namespace DirectX::SimpleMath;

namespace Hostile
{
    struct Transform;

    class PhysicsSys : public ISystem
    {

        //actual collisions info
        static std::vector<CollisionData> m_collisionData;

        // Map each entity to a vector of CollisionEvent objects.
        // This structure allows tracking multiple collision events per entity.
        static std::unordered_map<flecs::id_t, std::set<flecs::id_t>> m_previousFrameCollisions;
        static std::unordered_map<flecs::id_t, std::set<flecs::id_t>> m_currentFrameCollisions;        

        static float m_accumulatedTime;

        //detect
        static bool IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& _distVector, const float& _radSum, float& _distSqrd);
        static bool IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b);
        static bool IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _constraintOffset, float& _distance);
        static bool IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2);
        static bool IsColliding(const Transform& _tBox, const BoxCollider& _b, const PlaneCollider& _c);
		static float CalcPenetration(const Transform& _t1, const Transform& _t2, const Vector3& _colliderScale1, const Vector3& _colliderScale2, const Vector3& _colliderOffset1, const Vector3& _colliderOffset2, const Vector3& _axis);
        static void CalcOBBsContactPoints(const Transform& _t1, const Transform& _t2, CollisionData& _newContact, int _minPenetrationAxisIdx);
		static Vector3 GetLocalContactVertex(Vector3 _collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const _cmp);
        static Vector3 GetAxis(const Quaternion& _orientation, int _index);

        //resolve
        static void ApplyImpulses(flecs::entity _e1, flecs::entity _e2, float _jacobianImpulse, const Vector3& _r1, const Vector3& _r2, const Vector3& _direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static float ComputeTangentialImpulses(const flecs::entity& _e1, const flecs::entity& _e2, const Vector3& _r1, const Vector3& _r2, const Vector3& _tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2, const CollisionData& _collision);
        static void ApplyFrictionImpulses(flecs::entity _e1, flecs::entity _e2, const Vector3& _r1, const Vector3& _r2, const CollisionData& _collision, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static void ResolveCollisions();
        static void TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres);
        static void TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes);
        static void Integrate(flecs::iter& _it, Transform* _transform, Rigidbody* _rigidbody);

        static void AddCollisionData(const CollisionData& _data);
        static void ClearCollisionData();

        static constexpr float PLANE_OFFSET = 0.5f;
        static constexpr Vector3 UP_VECTOR{ 0, 1.f, 0 };//to convert quaternions to Vector3s

    public:

        PhysicsSys();
        virtual ~PhysicsSys() {}
        virtual void OnCreate(flecs::world& _world) override final;

        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& _type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& _type);
        void GuiDisplay(flecs::entity& _entity, const std::string& _type);
 
        //collision events
        static void HandleCollisionStart(flecs::id_t _entity1, flecs::id_t _entity2);
        static void HandleCollisionEnd(flecs::id_t _entity1, flecs::id_t _entity2);
        static void UpdateCollisionEvents();

        static constexpr int SOLVER_ITERS = 3;
    };
}
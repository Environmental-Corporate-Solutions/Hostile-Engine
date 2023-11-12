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
#include <utility>//std::pair

using namespace DirectX::SimpleMath;

namespace Hostile
{
    class Transform;

    class DetectCollisionSys : public ISystem
    {
    private:
        static bool IsColliding(const Transform& _t1, const Transform& _t2, const Vector3& distVector, const float& radSum, float& distSqrd);
        static bool IsColliding(const Transform& _tSphere, const SphereCollider& _s, const Transform& _tBox, const BoxCollider& _b);
        static bool IsColliding(const Transform& _tSphere, const Vector3& _constraintNormal, float _constraintOffset, float& distance);
        static bool IsColliding(const Transform& _t1, const BoxCollider& _b1, const Transform& _t2, const BoxCollider& _b2);
        static bool IsColliding(const Transform& _tBox, const BoxCollider& _b, const Constraint& _c);
		static float CalcPenetration(const Transform& t1, const Transform& t2, const Vector3& axis);
        static void CalcOBBsContactPoints(const Transform& t1, const Transform& t2, CollisionData& newContact, int minPenetrationAxisIdx);
		static Vector3 GetLocalContactVertex(Vector3 collisionNormal, const Transform& t, std::function<bool(const float&, const float&)> const cmp);
        static Vector3 GetAxis(const Quaternion& orientation, int index);

        static constexpr float PLANE_OFFSET = 0.5f;
        static constexpr Vector3 UP_VECTOR{ 0, 1.f, 0 };//to convert quaternions to Vector3s

        void DisplayMatrix3Editor(const char* label, Matrix3& matrix);
    public:
        virtual ~DetectCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;

        static void TestSphereCollision(flecs::iter& _it, Transform* _transforms, SphereCollider* _spheres);
        static void TestBoxCollision(flecs::iter& _it, Transform* _transforms, BoxCollider* _boxes);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
    };
}
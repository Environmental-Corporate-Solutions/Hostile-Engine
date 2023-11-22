//------------------------------------------------------------------------------
//
// File Name:	ResolveCollisionSys.h
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once

#include "directxtk12/SimpleMath.h"
#include "ISystem.h"
#include <optional>

using namespace DirectX;
namespace Hostile
{
    class Transform;
    class Rigidbody;
    class CollisionData;

    using DirectX::SimpleMath::Matrix;

    class ResolveCollisionSys : public ISystem
    {
    private:
        static void ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static float ComputeTangentialImpulses(const flecs::entity& e1, const flecs::entity& e2, const Vector3& r1, const Vector3& r2, const Vector3& tangent, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static void ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const Vector3& normal, Rigidbody* _rb1, Rigidbody* _rb2, const std::optional<Transform>& _t1, const std::optional<Transform>& _t2);
        static void OnUpdate(flecs::iter& _it, CollisionData* _collisionDatas);
        static void SendAndCleanupCollisionData(flecs::iter& _it, CollisionData* _collisionDatas);
    public:
        virtual ~ResolveCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
    };
}
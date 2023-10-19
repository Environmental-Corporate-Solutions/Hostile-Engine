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

using namespace DirectX;
namespace Hostile
{
    class CollisionData;
    class MassProperties;
    class Velocity;
    class Force;
    class InertiaTensor;
    class Transform;

    using DirectX::SimpleMath::Matrix;

    class ResolveCollisionSys : public ISystem
    {
    private:
        static float ComputeTangentialImpulses(const flecs::entity& e1, const flecs::entity& e2, const Vector3& r1, const Vector3& r2, const Vector3& tangent, bool isOtherEntityRigidBody);
        static void ApplyImpulses(flecs::entity e1, flecs::entity e2, float jacobianImpulse, const Vector3& r1, const Vector3& r2, const Vector3& direction, bool isOtherEntityRigidBody);
        static void ApplyFrictionImpulses(flecs::entity e1, flecs::entity e2, const Vector3& r1, const Vector3& r2, const Vector3& normal, bool isOtherEntityRigidBody);
        static void OnUpdate(flecs::iter& _it, CollisionData* _collisionDatas);
        static void CleanupCollisionData(flecs::iter& _it, CollisionData* _collisionDatas);
    public:
        virtual ~ResolveCollisionSys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components) override;
        void Read(flecs::entity& _object, nlohmann::json& _data);
        void GuiDisplay(flecs::entity& _entity);
    };
}
//------------------------------------------------------------------------------
//
// File Name:	ItegrateSys.h
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "directxtk/SimpleMath.h"
#include "ISystem.h"

using namespace DirectX::SimpleMath;
namespace Hostile
{
    class Transform;
    class MassProperties;
    class Velocity;
    class Force;
    class InertiaTensor;

    class IntegrateSys : public ISystem
    {
    private:
        static void UpdateTransformMatrix(const Transform& _transform, Matrix& _model);
    public:
        virtual ~IntegrateSys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        static void OnUpdate(flecs::iter& _it,Transform* transform, MassProperties* massProps, Velocity* velocities, Force* forces, Matrix* modelMatrices, InertiaTensor* insertiaTensor);
    };
}
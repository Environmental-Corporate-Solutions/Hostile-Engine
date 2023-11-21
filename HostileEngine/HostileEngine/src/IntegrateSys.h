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
#include "directxtk12/SimpleMath.h"
#include "ISystem.h"

using namespace DirectX::SimpleMath;
namespace Hostile
{
    class Transform;
    class Rigidbody;

    class IntegrateSys : public ISystem
    {
        static bool IsValid(const Vector3& vec);
        static bool IsValid(const Quaternion& quat);

    public:
        virtual ~IntegrateSys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        static void OnUpdate(flecs::iter& _it,Transform* _transform, Rigidbody* _rigidbody);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
    };
}
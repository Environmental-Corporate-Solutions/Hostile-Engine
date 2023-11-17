//------------------------------------------------------------------------------
//
// File Name:	PhysicsSimulationSys.h
// Author(s):	byeonggyu.park
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "directxtk12/SimpleMath.h"
#include "ISystem.h"
#include "Matrix3.h"//temp
#include "PhysicsProperties.h"

namespace Hostile
{
    //struct Damping {
    //    float linearDamping=.95f;
    //    float angularDamping=.7f;
    //};

    struct Gravity {
        //Vector3 direction = { 0, -9.81f, 0 };  
        Vector3 direction = { 0, -7.3575f, 0 }; //75% 
        //Vector3 direction = { 0, -4.905f, 0 }; //50% 
    };

    class GravitySys: public ISystem
    {
    private:

    public:
        virtual ~GravitySys() {}
        virtual void OnCreate(flecs::world& _world) override final;
        static void OnUpdate(flecs::iter& _it, Rigidbody* _rigidbody);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
    };
}
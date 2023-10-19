//------------------------------------------------------------------------------
//
// File Name:	ISystem.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "ISystemPtr.h"
#include "flecs.h"
#include "nlohmann/json.hpp"
namespace Hostile
{
  class ISystem
  {
  public:
    virtual ~ISystem() {};
    virtual void OnCreate(flecs::world& _world) = 0;
    virtual void Write(const flecs::entity& _entity,std::vector<nlohmann::json>& _components, const std::string& type) = 0;
    virtual void Read(flecs::entity& object, nlohmann::json& data, const std::string& type) = 0;
    virtual void GuiDisplay(flecs::entity& _entity, const std::string& type) = 0;

  };


}
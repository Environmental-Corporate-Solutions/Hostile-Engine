//------------------------------------------------------------------------------
//
// File Name:	Serializer.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "flecs.h"
#include "ISystemPtr.h"
#define REGISTER_TO_SERIALIZER(x,y)     \
ISerializer::Get().AddComponent(#x,y)  \


namespace Hostile
{

  class ISerializer
  {
  public:
    static ISerializer& Get();
    virtual void WriteEntityToFile(const flecs::entity& _current) = 0;
    virtual void AddComponent(const std::string _name, ISystemPtr _sys) = 0;
    virtual void WriteSceneToFile(const flecs::entity& _current) = 0;
  };
}
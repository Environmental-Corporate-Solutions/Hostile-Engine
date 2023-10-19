//------------------------------------------------------------------------------
//
// File Name:	Deseralizer.h
// Author(s):	Isiaah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "ISystemPtr.h"
#define REGISTER_TO_DESERIALIZER(x,y)    \
IDeseralizer::Get().AddComponent(#x,y);  \

namespace Hostile
{
  class IDeseralizer
  {
  public:
    static IDeseralizer& Get();
    virtual void ReadFile(const char* _filepath) = 0;
    virtual void AddComponent(const std::string _name, ISystemPtr _sys) = 0;
  };
}
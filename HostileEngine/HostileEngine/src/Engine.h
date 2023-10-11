//------------------------------------------------------------------------------
//
// File Name:	Engine.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "ISystemPtr.h"
#include "flecs.h"
#include "Serializer.h"
#define ADD_SYSTEM(x)                         \
    struct x##Adder                           \
    {                                         \
        x##Adder()                            \
        {                                     \
            IEngine::Get().Add(new x);        \
                                              \
        }                                     \
    };                                        \
    static x##Adder x##adder;                 \

#define ADD_COMPONENT(x,y)                  \
        IEngine::Get().AddComp(#x,y)        \




namespace Hostile
{
  class IEngine
  {
  public:
    static IEngine& Get();
    virtual void Add(ISystemPtr _pSys) = 0;
    virtual void Init() = 0;
    virtual flecs::world& GetWorld() = 0;
    virtual void Update() = 0;
    virtual void AddComp(const std::string& _name, ISystemPtr _sys) = 0;
    virtual Serializer& GetSerializer() = 0;
    virtual std::unordered_map<std::string, ISystemPtr>& GetMap() = 0;

    virtual flecs::entity& GetGravityPhase() = 0;
    virtual flecs::entity& GetDetectCollisionPhase() = 0;
    virtual flecs::entity& GetResolveCollisionPhase() = 0;
    virtual flecs::entity& GetIntegratePhase() = 0;
  private:
  };
}
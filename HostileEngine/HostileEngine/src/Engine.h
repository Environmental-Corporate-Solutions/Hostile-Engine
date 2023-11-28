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
#include "Scene.h"
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

struct Editor {};



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
    virtual float FrameRate() = 0;
    virtual Gui& GetGUI() = 0;
    virtual const bool IsGameRunning() = 0;
    virtual void SetGameRunning(bool _state) = 0;
    virtual flecs::entity CreateEntity(const std::string& _name = "New Actor") = 0;
    virtual Scene& AddScene(const std::string& _name) = 0;
    virtual Scene& GetScene(const std::string& _name) = 0;
    virtual Scene* GetCurrentScene() = 0;
    virtual bool IsSceneLoaded(const std::string& _name) = 0;
    virtual void SetCurrentScene(const std::string& _name) = 0;
    virtual void UnloadScene(int _id) = 0;

    virtual flecs::entity& GetGravityPhase() = 0;
    virtual flecs::entity& GetCollisionPhase() = 0;
    virtual flecs::entity& GetIntegratePhase() = 0;
  private:
  };
}
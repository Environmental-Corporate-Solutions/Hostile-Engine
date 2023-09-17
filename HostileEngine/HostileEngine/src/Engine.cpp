//------------------------------------------------------------------------------
//
// File Name:	Engine.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Engine.h"
#include <vector>
#include "flecs.h"
#include "ISystem.h"
#include "Gui.h"

namespace Hostile
{

  class Engine :public IEngine
  {
  public:
    void Add(ISystemPtr _pSys) override
    {
      m_allSystems.push_back(_pSys);
    }

    void Init() override
    {
      /* (custom phases)
       *  TransformSys ->   PhysicsSys  ->  DetectCollisionSys  ->  ResolveCollisionSys
      */
      m_physicsPhase = m_world.entity()
          .add(flecs::Phase)
          .depends_on(flecs::OnUpdate);

      m_detectCollisionPhase = m_world.entity()
          .add(flecs::Phase)
          .depends_on(m_physicsPhase);

      m_resolveCollisionPhase = m_world.entity()
          .add(flecs::Phase)
          .depends_on(m_detectCollisionPhase);

      for (ISystem* pSys : m_allSystems)
      {
        pSys->OnCreate(m_world);
      }
    }

    void Update()
    {
      m_world.progress();

      m_gui.RenderGui();
    }

    flecs::world& GetWorld() override
    {
      return m_world;
    }

    flecs::entity& GetPhysicsPhase() override {
        return m_physicsPhase;
    }
    flecs::entity& GetDetectCollisionPhase() override {
        return m_detectCollisionPhase;
    }
    flecs::entity& GetResolveCollisionPhase() override {
        return m_resolveCollisionPhase;
    }

  private:
    std::vector<ISystemPtr>m_allSystems;
    flecs::world  m_world;
    Gui m_gui;

    flecs::entity m_physicsPhase;
    flecs::entity m_detectCollisionPhase;
    flecs::entity m_resolveCollisionPhase;
  };




  IEngine& IEngine::Get()
  {
    static Engine engine;
    return engine;
  }
}
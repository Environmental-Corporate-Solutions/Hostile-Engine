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
#include <unordered_map>

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
       *  TransformSys ->   GravitySys  ->  DetectCollisionSys  ->  ResolveCollisionSys
      */
      m_world = std::make_unique<flecs::world>();
      m_gravityPhase = m_world->entity()
        .add(flecs::Phase)
        .depends_on(flecs::OnUpdate);

      m_detectCollisionPhase = m_world->entity()
        .add(flecs::Phase)
        .depends_on(m_gravityPhase);

      m_resolveCollisionPhase = m_world->entity()
        .add(flecs::Phase)
        .depends_on(m_detectCollisionPhase);

      m_integratePhase = m_world->entity()
        .add(flecs::Phase)
        .depends_on(m_resolveCollisionPhase);

      for (ISystem* pSys : m_allSystems)
      {
        pSys->OnCreate(*m_world);
      }
      m_gui.Init();
    }

    Engine()
    {
      
    }

    void Update()
    {
      m_world->progress();
      m_gui.RenderGui();

    }

    flecs::world& GetWorld() override
    {
      return *m_world;
    }

    flecs::entity& GetGravityPhase() override final {
      return m_gravityPhase;
    }
    flecs::entity& GetDetectCollisionPhase() override final {
      return m_detectCollisionPhase;
    }
    flecs::entity& GetResolveCollisionPhase() override final {
      return m_resolveCollisionPhase;
    }
    flecs::entity& GetIntegratePhase() override final {
      return m_integratePhase;
    }

    void AddComp(const std::string& _name, ISystemPtr _sys)
    {
      m_componentReg[_name] = _sys;
    }
    Serializer& GetSerializer()
    {
      return m_ser;
    }
    std::unordered_map<std::string, ISystemPtr>& GetMap()
    {
      return m_componentReg;
    }

  private:
    std::vector<ISystemPtr>m_allSystems;
    std::unique_ptr<flecs::world> m_world;
    std::unordered_map<std::string, ISystemPtr> m_componentReg;
    Gui m_gui;
    Serializer m_ser;


    flecs::entity m_gravityPhase;
    flecs::entity m_detectCollisionPhase;
    flecs::entity m_resolveCollisionPhase;
    flecs::entity m_integratePhase;
  };




  IEngine& IEngine::Get()
  {
    static Engine engine;
    return engine;
  }

}
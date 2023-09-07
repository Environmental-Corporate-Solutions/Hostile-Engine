//------------------------------------------------------------------------------
//
// File Name:	Engine.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "Engine.h"
#include <vector>
#include "flecs.h"
#include "ISystem.h"

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
      for (ISystem* pSys : m_allSystems)
      {
        pSys->OnCreate();
      }
    }
    flecs::world& GetWorld() override
    {
      return m_world;
    }
  private:
    std::vector<ISystemPtr>m_allSystems;
    flecs::world m_world;

  };




  IEngine& IEngine::Get()
  {
    static Engine engine;
    return engine;
  }
}
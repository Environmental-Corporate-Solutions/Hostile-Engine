//------------------------------------------------------------------------------
//
// File Name:	Deseralizer.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Deseralizer.h"
#include <fstream>
#include "Engine.h"
#include "ISystem.h"


namespace Hostile
{
  class Deseralizer : public IDeseralizer
  {
  public:
    void ReadFile(const char* _filepath)
    {
      
      std::ifstream file(_filepath);
      nlohmann::json json = nlohmann::json::parse(file);
      auto iter = json.begin();
      while (iter != json.end())
      {
        if (iter.key() == "Entity")
        {
          ReadEntity(iter.value());
          iter++;
        }
      }
    }
    void AddComponent(const std::string _name, ISystemPtr _sys)
    {
      m_map[_name] = _sys;
    }

  private:
    std::unordered_map<std::string, ISystemPtr> m_map;

    void ReadEntity(nlohmann::json& _obj)
    {
      flecs::entity entity = IEngine::Get().GetWorld().entity();
      std::string name = _obj["name"];
      entity.set_name(name.c_str());
      auto iter = _obj["Components"].begin();
      while (iter != _obj["Components"].end())
      {
        nlohmann::json comp = iter.value();
        std::string type = comp["Type"];
        assert(m_map.find(type) != m_map.end()); //component not register to deserialiser
        m_map[type]->Read(entity,comp, type);
        iter++;
      }
    }


  };
  IDeseralizer& IDeseralizer::Get()
  {
    static Deseralizer des;
    return des;
  }
}
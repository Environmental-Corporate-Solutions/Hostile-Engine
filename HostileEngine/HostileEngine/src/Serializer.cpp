//------------------------------------------------------------------------------
//
// File Name:	Serializer.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Serializer.h"
#include <fstream>
#include "ISystem.h"
namespace Hostile
{
  class Serializer : public ISerializer
  {
  public:
    Serializer()
    {

    }

    void WriteEntity(const flecs::entity& _current)
    {
      std::ofstream outfile("Test_Prefab.json");
      nlohmann::json obj = nlohmann::json::object();
      obj["name"] = _current.name();
      std::vector<nlohmann::json> comps;
      _current.each([&](flecs::id id) {
        if (!id.is_pair())
        {
          std::string name = id.entity().name();
          assert(m_map.find(name) != m_map.end()); // Component is not registered to serializer
          m_map[name]->Write(_current, comps, name);

        }
        });
      obj["Components"] = comps;
      nlohmann::json final;
      final["Entity"] = obj;
      outfile << final;
    }

    void AddComponent(const std::string _name, ISystemPtr _sys)
    {
      m_map[_name] = _sys;
    }

  private:
    std::unordered_map<std::string, ISystemPtr> m_map;
  };

  ISerializer& ISerializer::Get()
  {
    static Serializer serializer;
    return serializer;
  }
}
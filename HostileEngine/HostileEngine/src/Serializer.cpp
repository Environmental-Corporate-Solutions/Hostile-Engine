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
#include "nlohmann/json.hpp"
#include <fstream>
#include "ISystem.h"
namespace Hostile
{

  void Serializer::WriteEntity(const flecs::entity& _current,std::unordered_map<std::string,ISystemPtr>& _reg)
  {
    std::ofstream outfile("Test_Prefab.json");
    nlohmann::json file;
    file.push_back(_current.name());
    _current.each([&](flecs::id id) {
      if (!id.is_pair())
      {
        std::string name = id.entity().name();
        _reg[name]->Write(_current, file);
      }
      });

  }
}
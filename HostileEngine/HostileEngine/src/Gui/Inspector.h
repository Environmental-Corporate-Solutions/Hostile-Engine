//------------------------------------------------------------------------------
//
// File Name:	Inspector.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "ISystemPtr.h"
namespace Hostile
{
  typedef std::function<void(flecs::entity&, const std::string&)> DisplayFunc;
  typedef std::function<void(flecs::entity&)> AddFunc;
  typedef std::unordered_map<std::string, DisplayFunc> DisplayMap;
  typedef std::unordered_map<std::string, AddFunc> AddMap;


  class Inspector
  {
  public:
    void Render(int _id, DisplayMap& _display, AddMap& _add);
  };
}
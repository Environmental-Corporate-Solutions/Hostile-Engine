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
  class Inspector
  {
  public:
    void Render(int _id,std::unordered_map<std::string, ISystemPtr>& _map);
  };
}
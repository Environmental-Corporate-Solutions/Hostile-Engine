//------------------------------------------------------------------------------
//
// File Name:	Serializer.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------


#include "flecs.h"
#include "ISystemPtr.h"
namespace Hostile
{

  class Serializer
  {
  public:
    void WriteEntity(const flecs::entity&, std::unordered_map<std::string, ISystemPtr>& _reg);

  private:
  };
}
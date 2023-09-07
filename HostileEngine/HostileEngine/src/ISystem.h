//------------------------------------------------------------------------------
//
// File Name:	ISystem.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "ISystemPtr.h"
namespace Hostile
{
  class ISystem
  {
  public:
    virtual ~ISystem() {};
    virtual void OnCreate() = 0;


  };


}
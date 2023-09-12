//------------------------------------------------------------------------------
//
// File Name:	TransformSys.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "TransformSys.h"
#include "Engine.h"
namespace Hostile
{
  ADD_SYSTEM(TransformSys);
  void TransformSys::OnCreate()
  {
    IEngine::Get().GetWorld().system<Transform>();
  }

  void TransformSys::OnUpdate()
  {

  }

}
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

#include <iostream>
namespace Hostile
{
  ADD_SYSTEM(TransformSys);
  void TransformSys::OnCreate(flecs::world& _world)
  {
    _world.system<Transform>("TransformSys").kind(flecs::OnUpdate).iter(OnUpdate);
    auto e = _world.entity();
    e.add<Transform>();
  }

  void TransformSys::OnUpdate(flecs::iter _info ,Transform* _pTransforms)
  {
    for (int i : _info)
    {
      Transform& transform = _pTransforms[i];

    }
    //std::cout << "Transform update" << std::endl;
  }

}
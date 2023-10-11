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
    ADD_COMPONENT(Transform, this);

  }

  void TransformSys::OnUpdate(flecs::iter _info, Transform* _pTransforms)
  {
    for (int i : _info)
    {
      Transform& transform = _pTransforms[i];

      transform.matrix = XMMatrixTransformation(Vector3::Zero, Quaternion::Identity,
      transform.scale, Vector3::Zero, transform.orientation, transform.position);
    }
    //std::cout << "Transform update" << std::endl;
  }
  void TransformSys::Write(const flecs::entity& _entity, nlohmann::json& doc)
  {
    const Transform temp = *_entity.get<Transform>();
    doc.push_back("Transform");
    doc["Transform"]["position"] = temp.position.x;
  }
}
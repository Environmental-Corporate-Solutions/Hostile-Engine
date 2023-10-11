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
    REGISTER_TO_SERIALIZER(Transform, this);

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
  void TransformSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components)
  {
    const Transform temp = *_entity.get<Transform>();
    auto obj = nlohmann::json::object();
    obj["Type"] = "Transform";
    auto arr = nlohmann::json::array();
    arr.push_back(temp.position.x);
    arr.push_back(temp.position.y);
    arr.push_back(temp.position.z);
    obj["position"] = arr;

    auto rot = nlohmann::json::array();
    rot.push_back(temp.orientation.x);
    rot.push_back(temp.orientation.y);
    rot.push_back(temp.orientation.z);
    rot.push_back(temp.orientation.w);
    obj["Rotation"] = rot;
    _components.push_back(obj);
  }
}
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
    REGISTER_TO_DESERIALIZER(Transform, this);
    IEngine::Get().GetGUI().RegisterComponent("Transform", this);
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
    const Transform& temp = *_entity.get<Transform>();
    auto obj = nlohmann::json::object();
    obj["Type"] = "Transform";
    obj["Position"] = WriteVec3(temp.position);
    obj["Rotation"] = WriteVec4(temp.orientation);
    obj["Scale"] = WriteVec3(temp.scale);
    _components.push_back(obj);
  }
  void TransformSys::Read(flecs::entity& _object, nlohmann::json& _data)
  {
    Transform transform;
    transform.position = ReadVec3(_data["Position"]);
    transform.orientation = ReadVec4(_data["Rotation"]); 
    transform.scale = ReadVec3(_data["Scale"]);
    //_object.add<Transform>();
    _object.set<Transform>(transform);

  }
  void TransformSys::GuiDisplay(flecs::entity& _entity)
  {
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
      const Transform* transform = _entity.get<Transform>();
      Transform trans = *transform;
      ImGui::DragFloat3("Position", &trans.position.x, 0.1f);
      Vector3 rot = trans.orientation.ToEuler();
      rot *= 180.0f / PI;
      ImGui::DragFloat3("Rotation", &rot.x, 0.1f);
      rot *= PI / 180.0f;
      trans.orientation = Quaternion::CreateFromYawPitchRoll(rot);
      ImGui::DragFloat3("Scale", &trans.scale.x, 0.1f);
      _entity.set<Transform>(trans);
      ImGui::TreePop();
    }
  }
}
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
		_world.system<Transform>("EditorSys").kind(flecs::OnUpdate).kind<Editor>().iter(OnUpdate);
		REGISTER_TO_SERIALIZER(Transform, this);
		REGISTER_TO_DESERIALIZER(Transform, this);
		REGISTER_TO_SERIALIZER(ObjectName, this);
		REGISTER_TO_DESERIALIZER(ObjectName, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Transform",
			std::bind(&TransformSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[](flecs::entity& _entity) {_entity.add<Transform>(); });
	}																														

	void TransformSys::OnUpdate(flecs::iter _info, Transform* _pTransforms)
	{
		for (int i : _info)
		{
			Transform& transform = _pTransforms[i];
			if (_info.entity(i).parent().is_valid() && !_info.entity(i).parent().has<IsScene>()) {
				Transform parentTransform = TransformSys::GetWorldTransform(_info.entity(i).parent());
				Transform combinedTransform = CombineTransforms(parentTransform, transform);
				transform.matrix = XMMatrixTransformation(
					Vector3::Zero,
					Quaternion::Identity,
					combinedTransform.scale,        // S
					Vector3::Zero,
					combinedTransform.orientation,  // R
					combinedTransform.position      // T
				);
			}
			else {
				transform.matrix = XMMatrixTransformation(
					Vector3::Zero,
					Quaternion::Identity,
					transform.scale,
					Vector3::Zero,
					transform.orientation,
					transform.position
				);
			}
		}
	}
	void TransformSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{
		if (type == "ObjectName")
		{
			return;
		}
		const Transform& temp = *_entity.get<Transform>();
		auto obj = nlohmann::json::object();
		obj["Type"] = "Transform";
		obj["Position"] = WriteVec3(temp.position);
		obj["Rotation"] = WriteVec4(temp.orientation);
		obj["Scale"] = WriteVec3(temp.scale);
		_components.push_back(obj);
	}
	void TransformSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
		Transform transform;
		transform.position = ReadVec3(_data["Position"]);
		transform.orientation = ReadVec4(_data["Rotation"]);
		transform.scale = ReadVec3(_data["Scale"]);
		_object.set<Transform>(transform);
	}
	void TransformSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
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
		}

	}

  Transform TransformSys::CombineTransforms(const Transform& _worldParent, const Transform& _localTransform) 
  {
      Transform out;
      out.scale = _worldParent.scale * _localTransform.scale;

      out.orientation = _localTransform.orientation*_worldParent.orientation;
      out.orientation.Normalize();

      Vector3 scaledPos = _localTransform.position * _worldParent.scale;
      scaledPos = Vector3::Transform(scaledPos, _worldParent.orientation);

      out.position = _worldParent.position + scaledPos;

      return out;
  }

  Transform TransformSys::GetWorldTransformUtil(const flecs::entity& _e)
  {
	  if (!_e.parent().is_valid() || _e.parent().has<IsScene>())
	  {
		  return *_e.get<Transform>();
	  }
	  else
	  {
		  Transform worldParent = GetWorldTransformUtil(_e.parent());
		  return CombineTransforms(worldParent, *_e.get<Transform>());
	  }
  }

  Transform TransformSys::GetWorldTransform(const flecs::entity& _e)
  {
	  if (_e.has<IsScene>())
	  {
		  Log::Trace("Error");
	  }
      Transform worldSpaceTransform = GetWorldTransformUtil(_e);
      worldSpaceTransform.matrix=XMMatrixTransformation(Vector3::Zero, Quaternion::Identity,
          worldSpaceTransform.scale, Vector3::Zero, worldSpaceTransform.orientation, worldSpaceTransform.position);
      return worldSpaceTransform;
  }
}
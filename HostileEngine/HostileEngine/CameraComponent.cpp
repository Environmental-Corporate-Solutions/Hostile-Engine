#include "stdafx.h"
#include "CameraComponent.h"
#include "Engine.h"
#include "TransformSys.h"

namespace Hostile
{
	ADD_SYSTEM(CameraSys)



	/*CameraSys& CameraSys::operator=(const CameraSys& rhs)
	{
			return *this;
	}*/


	void CameraSys::OnCreate(flecs::world& _world)
	{
		_world.system<CameraData,Transform>("CameraSys").kind(flecs::OnUpdate).iter(OnUpdate);
		REGISTER_TO_SERIALIZER(Camera, this);
		REGISTER_TO_DESERIALIZER(Camera, this);

		flecs::entity e = _world.entity("CameraTest");
		e.add<CameraData>();
		
		
	}

	
	void CameraSys::OnUpdate(_In_ flecs::iter _info, _In_ CameraData* _pCamera, _In_ Transform* _pTransform)
	{
		
		for (auto it : _info)
		{
			//iterating through all camera entities we also need the transform.
			//otherwise when attached to a player it wont follow. however we can have multiple cameras attached to various entitys so if any of htose move the camera position needs to move as well.

			CameraData& cam = _pCamera[it];
			[[maybe_unused]] Transform&  _transform = _pTransform[it];
			UpdatePosition(cam, _pTransform[it].position);

			(cam.m_view_info.changed == true) ?
				DirectX::XMMatrixLookToRH
				(
					cam.m_view_info.m_position,
					cam.m_view_info.m_forward,
					cam.m_view_info.m_up
				):cam.m_view_matrix;

			if (cam.m_projection_info.changed)
			{
				 cam.m_projection_matrix = DirectX::XMMatrixPerspectiveFovRH
				 (
					cam.m_projection_info.m_fovY,
					cam.m_projection_info.m_aspectRatio,
					cam.m_projection_info.m_near,
					cam.m_projection_info.m_far
				);
			}
		}
	}

	void   CameraSys::Write(_In_ const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{

	}
	void CameraSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
		//does things. 
		
	}
	void CameraSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const CameraData* camera = _entity.get<CameraData>();
			CameraData cam = *camera;
			ImGui::DragFloat3("Position", &cam.m_view_info.m_position.x, 0.1f);
			//Position (view);
			//Vector3 rot = cam.orientation.ToEuler();
			//
			//ImGui::DragFloat3("Rotation", &rot.x, 0.1f);

			//Projection settings
			_entity.set<CameraData>(cam);
			ImGui::TreePop();
		}
	}
	Vector3 CameraSys::GetPosition(_In_ const  CameraData& _cam)
	{
		return _cam.m_view_info.m_position;
	}

	void CameraSys::UpdatePosition(CameraData& _cam, const Vector3 position)
	{
		_cam.m_view_info.m_position = position + _cam._offset;
		_cam.m_view_info.changed = true;
	}

	bool CameraSys::SetFOV(flecs::id _id, float _fov)
	{
		m_camera = GetCamera(_id);
		m_camera->m_projection_info.m_fovY=_fov;
		m_camera->m_projection_info.changed = true;

		return true;
	}

	std::shared_ptr<CameraData> CameraSys::GetCamera(flecs::id _id)
	{
		const flecs::world& _local_world = IEngine::Get().GetWorld();
		CameraData cam = *_local_world.get_alive(_id).get_mut<CameraData>();
		return std::make_shared<CameraData>(cam);
	}

	static CameraData* GetCamera(_In_ int _id)
	{
		flecs::world& _local_world = IEngine::Get().GetWorld();
		CameraData _cam = *_local_world.get_alive(_id).get_mut<CameraData>();
		return &_cam;
	}
}
#include "stdafx.h"
#include "CameraComponent.h"

#include "Camera.h"
#include "Engine.h"
#include "TransformSys.h"

namespace Hostile
{
	ADD_SYSTEM(CameraSys)


	void CameraSys::OnCreate(flecs::world& _world)
	{
		//editor Preupdate delcaration
		{
			_world.system<CameraData, Transform>("CameraSys")
				.term_at(2).optional()
				.kind(flecs::PreUpdate)
				.rate(.3f)
				.iter(OnUpdate);
		}

		{
			_world.system<CameraData, Transform>("CameraSys")
				.term_at(2).optional()
				.kind(flecs::PreUpdate)
				.rate(.3f)
				.iter(OnUpdate);
		}
		
		REGISTER_TO_SERIALIZER(CameraData, this);
		REGISTER_TO_DESERIALIZER(CameraData, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"CameraData",
			std::bind(&CameraSys::GuiDisplay, 
				this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<CameraData>(); });
		flecs::entity e = _world.entity("CameraTest");
		e.add<CameraData>();
		
	}

	
	void CameraSys::OnUpdate(_In_ flecs::iter _info, _In_ CameraData* _pCamera, _In_ Transform* _pTransform)
	{
		if (_info.is_set(2))
		{
			for (auto it : _info)
			{
				CameraData& cam = _pCamera[it];
				[[maybe_unused]] const Transform& _transform = _pTransform[it];
				UpdatePosition(cam, _transform.position);

			}

		}
		
		for (auto it : _info)
		{
			
			CameraData& cam = _pCamera[it];

			UpdateView(cam);
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
			const Transform* transform = _entity.get<Transform>();
			Transform _local_transform = *transform;
			const CameraData* camera = _entity.get<CameraData>();
			CameraData cam = *camera;
			ImGui::DragFloat3("Position", &_local_transform.position.x, 0.1f);
			UpdatePosition(cam, _local_transform.position);
			
			ImGui::DragFloat3("Offset", &cam._offset.x, 0.1f);
			UpdateOffset(cam, cam._offset);
			ImGui::Checkbox("Active", &cam.active);
			if(cam.active)
			{
				UpdateView(cam);
				UpdateProjection(cam);
				Camera::ChangeCamera(_entity.name().c_str());
			}
				
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

	void CameraSys::UpdateOffset(_In_ CameraData& _camera_component, _In_  Vector3 _offset)
	{
		_camera_component.m_view_info.m_position = GetPosition(_camera_component) + _offset;
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

	static CameraData& GetCamera(_In_ int _id)
	{
		flecs::world& _local_world = IEngine::Get().GetWorld();
		CameraData _cam = *_local_world.get_alive(_id).get_mut<CameraData>();
		return _cam;
	}

	void  CameraSys::UpdateView(_In_ CameraData& _data)
	{
		Vector3 globalUp = { 0, 1, 0 };
		if (_data.m_view_info.m_forward != Vector3{ 0, 1, 0 } && _data.m_view_info.m_forward != Vector3{ 0, -1, 0 })
		{
			_data.m_view_info.m_right = globalUp.Cross(_data.m_view_info.m_forward);
			_data.m_view_info.m_right.Normalize();

			_data.m_view_info.m_up = _data.m_view_info.m_forward.Cross(_data.m_view_info.m_right);
			_data.m_view_info.m_up.Normalize();
		}
		else
		{
			_data.m_view_info.m_up = _data.m_view_info.m_forward.Cross({ 1, 0, 0 });
			_data.m_view_info.m_up.Normalize();

			_data.m_view_info.m_right = _data.m_view_info.m_up.Cross(_data.m_view_info.m_forward);
			_data.m_view_info.m_right.Normalize();
		}

		_data.m_view_matrix = XMMatrixLookToRH(_data.m_view_info.m_position, _data.m_view_info.m_forward, _data.m_view_info.m_up);
	}


	void CameraSys::UpdateProjection(CameraData& _camera_data)
	{
		_camera_data.m_projection_matrix = XMMatrixPerspectiveFovRH(_camera_data.m_projection_info.m_fovY, _camera_data.m_projection_info.m_aspectRatio, _camera_data.m_projection_info.m_near, _camera_data.m_projection_info.m_far);
	}
	void CameraSys::SetCameraPosition(uint64_t _id, Vector3 _position)
	{
			auto& world = IEngine::Get().GetWorld(); 
		auto _cam = world.get_alive(_id).get_mut<CameraData>();
		_cam->m_view_info.m_position = _position;
		_cam->m_view_info.changed = true;
	}
}
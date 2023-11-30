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
		
			_world.system("CameraEditorSys")
				.kind(flecs::PreUpdate)
				.kind<Editor>()
				.rate(.3f)
				.iter([this](flecs::iter const& _info) {OnEdit(_info); });
		
			_world.system<Camera, Transform>("CameraSys")
				.term_at(2).optional()
				.kind(flecs::PreUpdate)
				.rate(.5f)
				.iter(OnUpdate);
		

		REGISTER_TO_SERIALIZER(Camera, this);
		REGISTER_TO_DESERIALIZER(Camera, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Camera",
			std::bind(&CameraSys::GuiDisplay, 
				this, std::placeholders::_1, std::placeholders::_2),
			[this](flecs::entity& _entity) { _entity.add<Camera>(); });
	
	}


	void CameraSys::OnUpdate(_In_ flecs::iter _info, _In_ Camera* _pCamera, _In_ Transform* _pTransform)
	{

		if (_info.is_set(2))
		{
			for (auto it : _info)
			{
				Camera& cam = _pCamera[it];
				[[maybe_unused]] const Transform& _transform = _pTransform[it];
				UpdatePosition(cam, _transform.position);

			}

		}

		for (auto it : _info)
		{
			_info.world().entity( ).parent();
			Camera& cam = _pCamera[it];
			
			
			UpdateView(cam);
			if (cam.active == true)
			{
				UpdateProjection(cam);

				SceneCamera::ChangeCamera(&cam);

			}
		}
		


	}

	
	void CameraSys::OnEdit(flecs::iter _info)
	{
		auto ent = _info.world().entity("Scene Camera");
		
			Camera* cam = ent.get_mut<Hostile::Camera>();
			[[maybe_unused]] Transform* _transform = ent.get_mut<Transform>();
			//UpdatePosition(*cam, _transform->position);

			UpdateProjection(*cam);
			UpdateView(*cam);
			SceneCamera::ChangeCamera(ent.id());
		

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
		bool is_open = ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("Camera Popup");
		}
		if (ImGui::BeginPopup("Camera Popup"))
		{
			if (ImGui::Button("Remove Component"))
			{
				_entity.remove<Camera>();
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
			ImGui::EndPopup();
		}
		if (is_open)
		{
			
			
			Camera* camera = _entity.get_mut<Camera>();
		
			
			if (_entity.has<Transform>())
			{
				const Transform* transform = _entity.get<Transform>();
				Transform _local_transform = *transform;
				ImGui::DragFloat3("Position", &_local_transform.position.x, 0.1f);
				UpdatePosition(*camera, _local_transform.position);
				_entity.set<Transform>(_local_transform);

			}else
			{
				ImGui::DragFloat3("Position", &camera->m_view_info.m_position.x, 0.1f);
			}
			
			ImGui::DragFloat3("Offset", &camera->_offset.x, 0.1f);
			UpdateOffset(*camera, camera->_offset);
			ImGui::Checkbox("Active", &camera->active);
			
		

			//Position (view);
			//Vector3 rot = cam.orientation.ToEuler();
			//
			//ImGui::DragFloat3("Rotation", &rot.x, 0.1f);

			//Projection settings
			_entity.set<Camera>(*camera);
			ImGui::TreePop();
		}
	}

	Vector3 CameraSys::GetPosition(_In_ const  Camera& _cam)
	{
		return _cam.m_view_info.m_position;
	}

	void CameraSys::UpdatePosition(Camera& _cam, const Vector3 position)
	{
		_cam.m_view_info.m_position = position + _cam._offset;
		
		_cam.m_view_info.changed = true;
	}

	void CameraSys::UpdateOffset(_In_ Camera& _camera_component, _In_  Vector3 _offset)
	{
		_camera_component.m_view_info.m_position = GetPosition(_camera_component) + _offset;
	}

	bool CameraSys::SetFOV(flecs::id _id, float _fov)
	{
		m_camera = GetCamera(_id);
		m_camera->m_projection_info.m_fovY = _fov;
		m_camera->m_projection_info.changed = true;

		return true;
	}

	std::shared_ptr<Camera> CameraSys::GetCamera(flecs::id _id)
	{
		const flecs::world& _local_world = IEngine::Get().GetWorld();
		Camera cam = *_local_world.get_alive(_id).get_mut<Camera>();
		return std::make_shared<Camera>(cam);
	}

	static Camera& GetCamera(_In_ int _id)
	{
		flecs::world& _local_world = IEngine::Get().GetWorld();
		Camera _cam = *_local_world.get_alive(_id).get_mut<Camera>();
		return _cam;
	}

	void  CameraSys::UpdateView(_In_ Camera& _data)
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


	void CameraSys::UpdateProjection(Camera& _camera_data)
	{
		_camera_data.m_projection_matrix = XMMatrixPerspectiveFovRH(_camera_data.m_projection_info.m_fovY, _camera_data.m_projection_info.m_aspectRatio, _camera_data.m_projection_info.m_near, _camera_data.m_projection_info.m_far);
	}

	void CameraSys::SetCameraPosition(uint64_t _id, Vector3 _position)
	{
		auto& world = IEngine::Get().GetWorld(); 
		auto _cam = world.get_alive(_id).get_mut<Camera>();
		_cam->m_view_info.m_position = _position;
		_cam->m_view_info.changed = true;
	}



}
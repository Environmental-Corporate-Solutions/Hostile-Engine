#include "stdafx.h"
#include "CameraComponent.h"
#include "Engine.h"


namespace Hostile
{
	ADD_SYSTEM(CameraSys)



	/*CameraSys& CameraSys::operator=(const CameraSys& rhs)
	{
			return *this;
	}*/


	void CameraSys::OnCreate(flecs::world& _world)
	{
		_world.system<Camera>("CameraSys").kind(flecs::OnUpdate).iter(OnUpdate);
		REGISTER_TO_SERIALIZER(Camera, this);
		REGISTER_TO_DESERIALIZER(Camera, this);
		IEngine::Get().GetGUI().RegisterComponent(
			"Camera",
			std::bind(&CameraSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[](flecs::entity& _entity) {_entity.add<Camera>(); });

	}

	void CameraSys::OnUpdate(_In_ flecs::iter _info, _In_ Camera* _pCamera)
	{
		for (auto it : _info)
		{
			Camera& cam = _pCamera[it];
			(cam.m_view_info.changed = true) ?
				DirectX::XMMatrixLookToRH
				(
					cam.m_view_info.m_position,
					cam.m_view_info.m_forward,
					cam.m_view_info.m_up
				):;

			if (cam.m_projection_info.changed)
			{
				 
			}
		}
	}

	void CameraSys::Write(_In_ const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{

	}
	void CameraSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
		Camera camera;
		
	}
	void CameraSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const Camera* camera = _entity.get<Camera>();
			Camera cam = *camera;
			ImGui::DragFloat3("Position", &cam.m_view_info.m_position.x, 0.1f);
			//Position (view);
			//Vector3 rot = cam.orientation.ToEuler();
			//
			//ImGui::DragFloat3("Rotation", &rot.x, 0.1f);

			//Projection settings
			_entity.set<Camera>(cam);
			ImGui::TreePop();
		}
	}
	Vector3 CameraSys::GetPosition(_In_ const  Camera& _cam)
	{
		return _cam.m_view_info.m_position;
	}

	static const CameraSys& GetCamera(_In_ int _id)
	{
		flecs::world& _local_world = IEngine::Get().GetWorld();
		Camera _cam = *_local_world.get_alive(_id).get_mut<Camera>();
		return &_cam;
	}
}
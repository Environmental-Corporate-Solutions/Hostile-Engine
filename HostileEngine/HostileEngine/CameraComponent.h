#pragma once
#include "ISystem.h"
#include <directxtk12/SimpleMath.h>

namespace Hostile
{
	struct Projection
	{
		float m_nearm =0;
		float m_far= 0;
		float m_aspectRatio = 0;
		float m_fovY = 0;
		bool changed = false;
	};

	struct View
	{
		Vector3 m_position{ 0.f,0.f,0.f };
		Vector3 m_up{ 0.f,1.f,0.f };
		Vector3 m_forward{0.f,0.f,-1.f};
		Vector3 m_right{ 1.f,0.f,0.f };
		bool changed = false;
	};

	struct Camera
	{
		Matrix m_view_matrix;
		Matrix m_projection_matrix;
		Projection m_projection_info;
		View m_view_info;
		float speed;
	};

	class CameraSys :public ISystem
	{
		virtual ~CameraSys() {}
		virtual void OnCreate(flecs::world& _world) override;
		static void OnUpdate(flecs::iter _info, Camera* _pC	amera);
		void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
		void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
		void GuiDisplay(flecs::entity& _entity, const std::string& type);
		Vector3 GetPosition(_In_ Camera& _cam);

		static CameraSys& GetCamera(_In_ int _id);
		
	};

}




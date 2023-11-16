#pragma once
#include "ISystem.h"
#include <directxtk12/SimpleMath.h>

namespace Hostile
{
	typedef struct Transform Transform;

	struct Projection
	{
		float m_near=0.1f;
		float m_far= 1000000;
		float m_aspectRatio = 1920.f/1080.f;
		float m_fovY = 45;
		bool changed = false;
	};

	struct View
	{
		Vector3 m_position{ 0.f,5.f,10.f };
		Vector3 m_up{ 0.f,1.f,0.f };
		Vector3 m_forward{0.f,0.f,-1.f};
		Vector3 m_right{ 1.f	,0.f,0.f };
		bool changed = false;
	};

	struct CameraData
	{
		Matrix m_view_matrix;
		Matrix m_projection_matrix;
		Projection m_projection_info;
		View m_view_info;
		float speed;
		Vector3 _offset = Vector3(0.0f, 0.0f, 0.0f);
		bool active = false;
	};

	class CameraSys :public ISystem
	{
	private:

		~CameraSys() final override{}
	public:

		virtual void OnCreate(flecs::world& _world) override;
		static void OnUpdate(flecs::iter _info, CameraData* _pCamera, Transform* _pTransform);
		void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
		void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
		void GuiDisplay(flecs::entity& _entity, const std::string& type);
		Vector3 GetPosition(_In_ const CameraData& _cam);
		static void UpdatePosition(CameraData& _cam, const Vector3 position);
		void UpdateOffset(CameraData& _camera_component, Vector3 _offset);
		bool SetFOV(_In_ flecs::id _id, _In_ float _fov);
		static CameraData& GetCamera(_In_ int _id);
		std::shared_ptr<CameraData> GetCamera(_In_ flecs::id _id);
		static void UpdateView(CameraData& _data);
		static void UpdateProjection(CameraData& _camera_data);
		static void SetCameraPosition(uint64_t _id, Vector3 _position);
	private:
		std::shared_ptr<CameraData> m_camera;
		bool _is_camera_active = false;

	};

}




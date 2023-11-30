/**
 * @file CameraComponent.h
 * @brief defines the systems and components for the Engine and player camera
 *
 * The camera framework provides the base necessities for camera injection and
 * operation in Hostile Engine. 
 *
 * @TODO: Strip out projection from the base camera and only add the component if
 * actually wanted for the parent. possible use cases: Scopes, Cutscene cameras.
 * panning out and zooming in. 
 */

#pragma once
#include "ISystem.h"
#include <directxtk12/SimpleMath.h>

namespace Hostile
{
	typedef struct Transform Transform;

	/* Projection struct
		 Returns a Right-Handed Projection matrix using DX12:SimpleMath
	*/
	struct Projection
	{
		float m_near=0.1f;
		float m_far= 1000000;
		float m_aspectRatio = 1920.f/1080.f;
		float m_fovY = 45;
		bool changed = true;
	};

	/* View Matrix Properties	*/
	struct View
	{
		Vector3 m_position{ 0.f,5.f,10.f };
		Vector3 m_up{ 0.f,1.f,0.f };
		Vector3 m_forward{0.f,0.f,-1.f};
		Vector3 m_right{ 1.f	,0.f,0.f };
		bool changed = true;
	};

	/* Camera Data  - 
	Provides all necessary components for BASIC camera rendering
	offset - allows 3rd person or first person views.
	*/
	struct CameraData
	{
		Matrix m_view_matrix;					/* view matrix */
		Matrix m_projection_matrix;   /* 4x4 XMMatrix projection*/
		Projection m_projection_info; /* projection struct acces*/
		View m_view_info;							/* view matrix struct access*/
		float speed;									/* Camera speed changes*/
		Vector3 _offset = Vector3(0.0f, 4.0f, 6.0f); /* offset for viewing*/
		bool active = false;										/* active status*/
	};

	class CameraSys :public ISystem
	{
	private:

		~CameraSys() final override{}
	public:

		virtual void OnCreate(flecs::world& _world) override;
		static void OnUpdate(
            flecs::iter _info, 
            CameraData* _pCamera, 
            Transform* _pTransform
        );
		/* static function systems*/
	public:
		static void OnEdit( flecs::iter _info);
		static void UpdatePosition(CameraData& _cam, const Vector3 position);
		static CameraData& GetCamera(_In_ int _id);
		static void UpdateView(CameraData& _data);
		static void UpdateProjection(CameraData& _camera_data);
		static void SetCameraPosition(uint64_t _id, Vector3 _position);

		/* GUI functions */
	public:
		void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
		void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
		void GuiDisplay(flecs::entity& _entity, const std::string& type);

		Vector3 GetPosition(_In_ const CameraData& _cam);
		void UpdateOffset(CameraData& _camera_component, Vector3 _offset);
		bool SetFOV(_In_ flecs::id _id, _In_ float _fov);
		std::shared_ptr<CameraData> GetCamera(_In_ flecs::id _id);
	private:
		std::shared_ptr<CameraData> m_camera;
		bool _is_camera_active = false;

	};

}




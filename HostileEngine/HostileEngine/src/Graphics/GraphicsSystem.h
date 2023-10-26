#pragma once
#include <string>
#include "IGraphics.h"

#include "TransformSys.h"
#include <map>

#include <imgui.h>

namespace Hostile
{
	struct LightData
	{
		Vector3 color;
		LightID id;
	};

	struct InstanceData
	{
		InstanceID id;

		MeshID meshId;
		std::string meshName;

		MaterialID materialId;
		std::string materialName;
	};

	class GraphicsSys : public ISystem
	{
	private:
		std::map<std::string, MeshID, std::less<>> m_meshMap;
		std::map<std::string, MaterialID, std::less<>> m_materialMap;
		std::vector<IRenderTargetPtr> m_renderTargets;
		std::vector<std::shared_ptr<DepthTarget>> m_depthTargets;

		ImVec2 m_currDragDelta;
		Camera m_camera;

		flecs::query<InstanceData, Transform> m_geometryPass;
		flecs::query<LightData, Transform>    m_lightPass;

		InstanceData ConstructInstance(const std::string _mesh, const std::string _material);
		bool m_is_moving = false;

	public:
		~GraphicsSys() override = default;
		void OnCreate(flecs::world& _world) override;
		void PreUpdate(flecs::iter const& _info);
		void OnUpdate(flecs::iter const& _info) const;
		void OnUpdate(InstanceData const& _instance, Transform const& _transform) const;
		void PostUpdate(flecs::iter const& _info);

		void AddMesh(flecs::iter& _info);
		void AddTexture(flecs::iter& _info);
		void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
		void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
		void GuiDisplay(flecs::entity& _entity, const std::string& type);
	};
}
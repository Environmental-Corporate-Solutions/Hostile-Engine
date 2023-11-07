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
        UINT id;
    };

    class GraphicsSys : public ISystem
    {
    private:
        std::unordered_map<std::string, VertexBufferPtr> m_mesh_map;
        std::unordered_map<std::string, MaterialPtr> m_material_map;
        std::vector<IRenderTargetPtr> m_render_targets;
        std::vector<IReadBackBufferPtr> m_readback_buffers;
        std::vector<std::shared_ptr<DepthTarget>> m_depth_targets;

        ImVec2 m_curr_drag_delta;
        Camera m_camera;

        flecs::query<InstanceData, Transform> m_geometry_pass;
        flecs::query<LightData, Transform>    m_light_pass;
        UINT light_id = 0;

        bool m_material_edit = false;

        MaterialPtr m_outline_material;
        PipelinePtr m_outline_pipeline;
        VertexBufferPtr m_outline_buffer;

        InstanceData ConstructInstance(const std::string _mesh, const std::string _material, const UINT32 _id);

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
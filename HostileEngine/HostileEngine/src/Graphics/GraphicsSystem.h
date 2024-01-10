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
    };

    class GraphicsSys : public ISystem
    {
    private:
        enum GizmoMode
        {
            None,
            Translate,
            Rotate,
            Scale
        };
        std::vector<IRenderTargetPtr> m_render_targets;
        std::vector<IReadBackBufferPtr> m_readback_buffers;
        std::vector<std::shared_ptr<DepthTarget>> m_depth_targets;

        ImVec2 m_curr_drag_delta;
        Camera m_camera;

        flecs::query<Renderer, Transform> m_geometry_pass;
        flecs::query<LightData, Transform>    m_light_pass;
        UINT light_id = 0;

        bool m_material_edit = false;
        bool m_graphics_settings = false;
        Vector4 m_ambient_light = { 1, 1, 1, 0.1f };
        bool m_is_view_clicked;
        GizmoMode m_gizmo = GizmoMode::Translate;
        bool m_translate = false;
        bool m_rotate = false;
        bool m_scale = false;


	public:
		~GraphicsSys() override = default;
		void OnCreate(flecs::world& _world) override;
		void PreUpdate(flecs::iter const& _info);
		void OnUpdate(flecs::iter const& _info) const;
		void OnUpdate(Renderer const& _instance, Transform const& _transform) const;
		void PostUpdate(flecs::iter const& _info);

        void AddMesh(flecs::iter& _info);
        void AddTexture(flecs::iter& _info);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type) override;
        void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
        void GuiDisplay(flecs::entity& _entity, const std::string& type);
    };
}
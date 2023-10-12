#pragma once
#include <string>
#include "IGraphics.h"

#include "TransformSys.h"
#include <map>

#include <imgui.h>

namespace Hostile
{
    struct Mesh
    {
        std::string meshName = "";
        size_t meshIndex = 0;
    };

    struct Texture
    {
        std::string textureName = "";
        size_t textureIndex = 0;
    };

    struct Material
    {
        std::string textureName = "";
        size_t textureIndex = -1;
    };

    class GraphicsSys : public ISystem
    {
    private:
        std::map<std::string, MeshID> m_meshMap;
        std::vector<IRenderTargetPtr> m_renderTargets;
        std::vector<std::shared_ptr<DepthTarget>> m_depthTargets;

        ImVec2 m_currDragDelta;
        Camera m_camera;

        flecs::query<InstanceID, Transform> m_geometryPass;

    public:
        ~GraphicsSys() override = default;
        void OnCreate(flecs::world& _world) override;
        void PreUpdate(flecs::iter const& _info);
        void OnUpdate(flecs::iter const& _info);
        void OnUpdate(flecs::iter const& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes);
        void OnUpdate(InstanceID& _instance, Transform& _transform);
        void PostUpdate(flecs::iter const& _info);

        void AddMesh(flecs::iter& _info);
        void AddTexture(flecs::iter& _info);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components) override;
    };
}
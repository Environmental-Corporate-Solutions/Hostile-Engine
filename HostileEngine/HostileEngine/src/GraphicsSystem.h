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
        std::vector<std::unique_ptr<GeometricPrimitive>> m_meshes;
        std::vector<std::unique_ptr<GTexture>> m_textures;
        std::map<std::string, size_t, std::less<>> m_meshMap;
        std::map<std::string, size_t, std::less<>> m_textMap;

        std::vector<std::shared_ptr<RenderTarget>> m_renderTargets;
        std::vector<std::shared_ptr<DepthTarget>> m_depthTargets;

        ImVec2 m_currDragDelta;
        Camera m_camera;

        flecs::query<Transform, Mesh, Material> m_geometryPass;

    public:
        ~GraphicsSys() override = default;
        void OnCreate(flecs::world& _world) override;
        void PreUpdate(flecs::iter const& _info);
        void OnUpdate(flecs::iter const& _info);
        void OnUpdate(flecs::iter const& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes);
        void OnUpdate(Transform& _transform, Mesh& _mesh, Material& _material);
        void PostUpdate(flecs::iter const& _info);

        void AddMesh(flecs::iter& _info);
        void AddTexture(flecs::iter& _info);
        void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components) override;
    };
}
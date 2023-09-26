#pragma once
//#include "ResourceLoader.h"
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

    class GraphicsSys : public ISystem
    {
    private:
        std::vector<std::unique_ptr<GeometricPrimitive>> m_meshes;
        std::vector<std::unique_ptr<MoltenTexture>> m_textures;
        std::map<std::string, size_t> m_meshMap;
        std::map<std::string, size_t> m_textMap;

        std::vector<std::shared_ptr<MoltenRenderTarget>> m_renderTargets;

        ImVec2 m_currDragDelta;
        Camera m_camera;

        flecs::query<Transform, Mesh> m_geometryPass;

    public:
        virtual ~GraphicsSys() {}
        void OnCreate(flecs::world& _world) override;
        //void OnUpdate(flecs::iter& _info, Mesh* _pMeshes);
        void PreUpdate(flecs::iter& _info);
        void OnUpdate(flecs::iter& _info);
        void OnUpdate(flecs::iter& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes);
        void OnUpdate(flecs::iter& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes, flecs::column<Texture>& _pTextures);
        void PostUpdate(flecs::iter& _info);
        //void OnUpdate(flecs::iter& _info, Skeleton* _pSkeletons, Mesh* _pMeshes, Animation* _pAnimations);

        void AddMesh(flecs::iter& _info);
        void AddTexture(flecs::iter& _info);
    };
}
#pragma once
//#include "ResourceLoader.h"
#include <string>
#include "IGraphics.h"

#include "TransformSys.h"
#include <map>

namespace Hostile
{
    struct Mesh
    {
        std::string meshName;
        size_t meshIndex;
    };

    struct Texture
    {
        std::string textureName;
        size_t textureIndex;
    };

    class GraphicsSys : public ISystem
    {
    private:
        std::map<std::string, std::unique_ptr<GeometricPrimitive>> m_meshes;
        std::map<std::string, ComPtr<ID3D12Resource>> m_textures;

    public:
        virtual ~GraphicsSys() {}
        void OnCreate(flecs::world& _world) override;
        //void OnUpdate(flecs::iter& _info, Mesh* _pMeshes);
        void OnUpdate(flecs::iter& _info);
        void OnUpdate(flecs::iter& _info, Transform* _pTransforms, Mesh* _pMeshes);
        //void OnUpdate(flecs::iter& _info, Skeleton* _pSkeletons, Mesh* _pMeshes, Animation* _pAnimations);

        void AddMesh(flecs::iter& _info);
        void AddTexture(flecs::iter& _info);
    };
}
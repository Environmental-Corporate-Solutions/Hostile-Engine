#pragma once
#include <array>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <array>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/DirectXHelpers.h>
#include <DirectXMath.h>
#include <wrl.h>

#include "Geometry.h"

#include "GpuDevice.h"

#define MAX_BONES 200
#define MAX_WEIGHTS 8

namespace Hostile
{
    using namespace Microsoft::WRL;
    using namespace DirectX::SimpleMath;
    using namespace DirectX;

    
    struct AnimationNode
    {
        struct VectorKey
        {
            float time;
            Vector3 value;
        };

        struct QuatKey
        {
            float time;
            Quaternion value;
        };

        std::string nodeName;
        size_t nodeIndex;
        std::vector<VectorKey> positionKeys;
        std::vector<QuatKey> rotationKeys;
        std::vector<VectorKey> scalingKeys;
    };

    struct Animation
    {
        std::string name = "";
        float duration = 0;
        float ticksPerSec = 0;
        float timeInSeconds = 0;
        float numChannels = 0;
        std::vector<AnimationNode> nodes;
    };

    struct Skeleton
    {
        std::vector<Matrix> boneMatrices;
        std::vector<UINT> joints{};
        std::vector<Matrix> inverseBindMatrices{};
        UINT skeleton = 0;
        Matrix globalInverseTransform;
    };

    struct MeshData
    {
        struct Entry
        {
            std::string name;
            size_t baseVertex;
        };
        std::vector<Entry> entries{};
        std::vector<SkinnedVertex> vertices{};
        std::vector<uint16_t> indices{};
    };

    struct Node
    {
        std::string name = "";
        UINT        camera = UINT(-1);
        UINT        mesh = UINT(-1);
        UINT        skin = UINT(-1);

        Vector3     translation{};
        Quaternion  rotation = Quaternion::Identity;
        Vector3     scale = Vector3::One;
        Matrix      matrix = Matrix::Identity;

        std::vector<UINT> children{};
    };

    struct SceneData
    {
        std::string name;
        std::vector<Node> nodes;
        MeshData meshData;
        Skeleton skeleton;
        std::vector<Animation> animations;
    };

    class IGraphicsResource
    {
    protected:
        IGraphicsResource(GpuDevice& _device, const std::string& _name) 
            : m_name(_name), m_device(_device) {}
        std::string m_name = "";
        GpuDevice& m_device;
    public:
        using TypeID = uint64_t;
        virtual ~IGraphicsResource() {}
        const std::string& Name() const { return m_name; }
        GpuDevice& GetDevice() const { return m_device; }
    };
    using IGraphicsResourcePtr = std::shared_ptr<IGraphicsResource>;

    using D3D12ResourcePtr = Microsoft::WRL::ComPtr<ID3D12Resource>;

    class ResourceLoader;
    class ResourceLoader
    {
    public:
        template<class ResourceType> 
        std::shared_ptr<ResourceType> 
            GetOrLoadResource(const std::string& _name)
        {
            static_assert(
                std::is_base_of<IGraphicsResource, ResourceType>::value,
                "Resource Type is not derived from IGraphicsResource");
            if (m_resource_cache[ResourceType::TypeID()].find(_name) 
                != m_resource_cache[ResourceType::TypeID()].end())
            {
                return std::dynamic_pointer_cast<ResourceType>
                    (m_resource_cache[ResourceType::TypeID()][_name]);
            }
            std::shared_ptr<ResourceType> resource 
                = ResourceType::Create(m_device, _name);
            m_resource_cache[ResourceType::TypeID()][_name] = 
                std::dynamic_pointer_cast<IGraphicsResource>(resource);
            return resource;
        }

        template<class ResourceType>
        std::shared_ptr<ResourceType>
            GetResource(const std::string& _name)
        {
            static_assert(
                std::is_base_of<IGraphicsResource, ResourceType>::value,
                "Resource Type is not derived from IGraphicsResource");
            if (m_resource_cache[ResourceType::TypeID()].find(_name)
                != m_resource_cache[ResourceType::TypeID()].end())
            {
                return std::dynamic_pointer_cast<ResourceType>
                    (m_resource_cache[ResourceType::TypeID()][_name]);
            }
            return nullptr;
        }

        static void Init(GpuDevice& _device);
        static ResourceLoader& Get();
        
        explicit ResourceLoader(GpuDevice& _device) : m_device(_device) {}
    private:
        static std::unique_ptr<ResourceLoader> g_instance;

        struct ResourceEntry
        {
            std::string path;
            IGraphicsResourcePtr entry;
        };
        GpuDevice& m_device;
        using GraphicsResourceMap =
            std::unordered_map<std::string, IGraphicsResourcePtr>;
        using GraphicsResourceMapMap =
            std::unordered_map<IGraphicsResource::TypeID, GraphicsResourceMap>;

        GraphicsResourceMapMap m_resource_cache{};
        friend class Graphics;
    };

    SceneData LoadSceneFromFile(std::string& _filepath);
    SceneData LoadAnimationFromFile(std::string& _filepath);
}
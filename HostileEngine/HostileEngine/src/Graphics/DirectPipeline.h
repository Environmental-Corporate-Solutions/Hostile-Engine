#pragma once
#include "GraphicsHelpers.h"
#include <map>
#include <variant>
#include "GpuDevice.h"

#include "GraphicsTypes.h"
#include "RenderTarget.h"

#include "ResourceLoader.h"

using namespace Hostile;

namespace Hostile
{
    // forward declerations
    struct PipelineNode;
    struct PipelineNodeResource;
    using PipelineNodeHandle = std::weak_ptr<PipelineNode>;
    using PipelineNodePtr = std::shared_ptr<PipelineNode>;

    using PipelineNodeResourcePtr = std::shared_ptr<PipelineNodeResource>;
    
    struct PipelineNodeResource
    {
        struct Description
        {
            Vector2 dimensions;
            DXGI_FORMAT format;
            UINT offset = 0;
            UINT size = 0;
        };
        RenderTargetPtr resource;
        
        static DXGI_FORMAT FormatFromString(std::string const& _str);

        std::string name = "";
        std::string alias = "";
        Description desc;

        PipelineNodeHandle producer;
        PipelineNodeHandle deleter;
        int32_t ref_count = 0;
    };

    struct PipelineNodeResourceHandle
    {
        enum class Type
        {
            RENDER_TARGET,
            SHADER_RESOURCE,
            REFERENCE,
            INVALID
        };
        static Type TypeFromString(std::string const& _str);
        Type type = Type::INVALID;
        std::weak_ptr<PipelineNodeResource> resource;
    };

    struct PipelineNode
    {
        enum class Frequency
        {
            PER_OBJECT,
            PER_LIGHT,
            PER_FRAME,
            INVALID
        };
        static Frequency FrequencyFromString(std::string& _str);
        std::string name;

        Frequency frequency = Frequency::INVALID;
        PipelinePtr pipeline;

        std::vector<PipelineNodeResourceHandle> inputs;
        std::vector<PipelineNodeResourceHandle> outputs;

        std::vector<PipelineNodeHandle> edges;
    };

    class PipelineNodeGraph
    {
    public:
        void Read(std::string _name, std::string _path = "");
        void Compile(GpuDevice& _device, PipelineMap& _pipelines);
        PipelineNodeResourcePtr FindNodeResource(std::string _name);
    private:
        std::vector<PipelineNodePtr> m_nodes;
        std::unordered_map<std::string, PipelineNodeResourcePtr> m_resources;
        std::string m_name = "";
        ComPtr<ID3D12Heap> m_heap;
    };
}
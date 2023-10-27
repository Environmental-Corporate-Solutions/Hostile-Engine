#pragma once
#include "GraphicsHelpers.h"
#include <map>
#include <variant>
#include "GpuDevice.h"

using namespace Hostile;

namespace Hostile
{
    class Pipeline
    {
    public:
        enum class InputLayout
        {
            PRIMITIVE,
            SKINNED,
            FRAME,
            COUNT
        };

        enum class Buffer
        {
            SCENE,
            MATERIAL,
            OBJECT
        };

        struct MaterialInput
        {
            enum class Type
            {
                TEXTURE = 0,
                FLOAT,
                FLOAT2,
                FLOAT3,
                FLOAT4,
                INVALID
            };
            static Type TypeFromString(std::string const& _str);
            static constexpr std::array typeSizes = {
                sizeof(float), sizeof(Vector2), sizeof(Vector3),
                sizeof(Vector4)
            };
            std::string name;
            Type type;
            std::variant<Texture, float, Vector2, Vector3, Vector4> value;
        };

        struct Material
        {
            std::string name;
            std::vector<MaterialInput> m_materialInputs;
            size_t size;
            std::string pipeline;
        };
        Pipeline(void) = default;
        
        const InputLayout& GetInputLayout() const;
        const std::vector<Buffer>& Buffers() const;
        const std::vector<MaterialInput>& MaterialInputs() const;
        const size_t MaterialInputsSize() const;
        const std::string& Name() const;

        void Bind(CommandList& _cmd) const;

        static Pipeline Create(GpuDevice& _gpu, std::string _name);
    private:
        void Init(GpuDevice& _gpu, std::string _name);
    private:
        InputLayout m_inputLayout = InputLayout::PRIMITIVE;
        std::vector<Buffer> m_buffers{};
        std::vector<MaterialInput> m_materialInputs;
        size_t m_materialInputsSize = 0;

        ComPtr<ID3D12PipelineState> m_pipeline;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        std::string m_name;
    };

    // forward declerations
    struct PipelineNode;
    struct PipelineNodeResource;
    using PipelineNodeHandle = std::weak_ptr<PipelineNode>;
    using PipelineNodePtr = std::shared_ptr<PipelineNode>;

    using PipelineNodeResourceHandle = std::weak_ptr<PipelineNodeResource>;
    using PipelineNodeResourcePtr = std::shared_ptr<PipelineNodeResource>;
    
    struct PipelineNodeResource
    {
        enum class Type
        {
            RENDER_TARGET,
            SHADER_RESOURCE,
            REFERENCE,
            INVALID
        };
        struct Description
        {
            Vector2 dimensions;
            DXGI_FORMAT format;
        };
        static Type TypeFromString(std::string const& _str);
        static DXGI_FORMAT FormatFromString(std::string const& _str);
        Type type = Type::INVALID;
        Description desc;

        PipelineNodeHandle producer;
        PipelineNodeResourceHandle outputHandle;

        int32_t refCount = 0;
        std::string name = "";
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

        Frequency frequency = Frequency::INVALID;
        std::shared_ptr<Pipeline> pipeline;

        std::vector<PipelineNodeResourceHandle> inputs;
        std::vector<PipelineNodeResourceHandle> outputs;

        std::vector<PipelineNodeHandle> edges;

        std::string name;
    };

    class PipelineNodeGraph
    {
    public:
        void Read(std::string _name, std::string _path = "");
        void Compile();
        PipelineNodeResourcePtr FindNodeResource(std::string _name);
    private:
        std::vector<PipelineNodePtr> m_nodes;
        std::vector<PipelineNodeResourcePtr> m_resources;
        std::string m_name = "";
    };
}
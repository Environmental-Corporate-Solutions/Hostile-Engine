#include "stdafx.h"
#include "DirectPipeline.h"

#include <deque>
#include <map>

#include <directxtk12/CommonStates.h>
#include <directxtk12/EffectPipelineStateDescription.h>

#include "Geometry.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace Hostile
{
    static PipelineNodeResource ReadResource(nlohmann::json const& _data)
    {
        PipelineNodeResource resource{};
        resource.name = _data["Name"].get<std::string>();

        std::string type = _data["Type"].get<std::string>();
        resource.type = PipelineNodeResource::TypeFromString(type);
        if (resource.type == PipelineNodeResource::Type::INVALID)
            Log::Error("PipelineNodeResource " + resource.name + " unknown Type: " + type);

        if (_data.contains("Dimensions"))
            resource.desc.dimensions = { _data["Dimensions"][0].get<float>(), _data["Dimensions"][1].get<float>() };

        if (_data.contains("Format"))
            resource.desc.format = PipelineNodeResource::FormatFromString(_data["Format"].get<std::string>());

        return resource;
    }

    void Hostile::PipelineNodeGraph::Read(std::string _name, std::string _path)
    {
        using namespace nlohmann;
        std::ifstream stream(_path + _name);

        json data = json::parse(stream);

        m_name = data["Name"];

        auto const& nodes = data["PipelineNodes"];
        for (auto const& pipelineNode : nodes)
        {
            PipelineNodePtr pNode = std::make_shared<PipelineNode>();

            pNode->name = pipelineNode["Name"].get<std::string>();
            std::string frequency = pipelineNode["Frequency"].get<std::string>();
            pNode->frequency = PipelineNode::FrequencyFromString(frequency);
            if (pNode->frequency == PipelineNode::Frequency::INVALID)
                Log::Error("PipelineNode " + pNode->name + " unknown Frequency: " + frequency);
            m_nodes.push_back(pNode);

            for (json const& input : pipelineNode["Inputs"])
            {
                PipelineNodeResource resource = ReadResource(input);
                m_resources.push_back(std::make_shared<PipelineNodeResource>(resource));
                pNode->inputs.push_back(m_resources.back());
            }

            for (auto const& output : pipelineNode["Outputs"])
            {
                PipelineNodeResource resource = ReadResource(output);
                resource.producer = m_nodes.back();
                m_resources.push_back(std::make_shared<PipelineNodeResource>(resource));
                pNode->outputs.push_back(m_resources.back());
            }
        }
    }

    void PipelineNodeGraph::Compile()
    {
        for (auto& node : m_nodes)
        {
            for (auto& input : node->inputs)
            {
                if (PipelineNodeResourcePtr resource = input.lock())
                {
                    if (PipelineNodeResourcePtr r = FindNodeResource(resource->name))
                    {
                        resource->outputHandle = r;
                        resource->desc = r->desc;
                        resource->producer = r->producer;
                        if (PipelineNodePtr n = r->producer.lock())
                            n->edges.push_back(node);
                    }
                }
            }
        }

        std::unordered_map<std::string, uint8_t> visited;
        std::deque<PipelineNodeHandle> stack;
        std::vector<PipelineNodePtr> sortedNodes;
        for (auto const& node : m_nodes)
        {
            stack.push_back(node);
            visited.emplace(node->name, 0);
        }

        while (!stack.empty())
        {
            PipelineNodePtr nodeHandle = stack.back().lock();
            if (visited[nodeHandle->name] == 2)
            {
                stack.pop_back();
                continue;
            }

            if (visited[nodeHandle->name] == 1)
            {
                visited[nodeHandle->name] = 2;

                sortedNodes.push_back(nodeHandle);

                stack.pop_back();
                continue;
            }

            visited[nodeHandle->name] = 1;
            if (nodeHandle->edges.size() == 0)
                continue;

            for (auto& r : nodeHandle->edges)
            {
                if (!visited[r.lock()->name])
                    stack.push_back(r);
            }
        }

        m_nodes.clear();
        while (!sortedNodes.empty())
        {
            m_nodes.push_back(sortedNodes.back());
            sortedNodes.pop_back();
        }
    }

    PipelineNodeResourcePtr PipelineNodeGraph::FindNodeResource(std::string _name)
    {
        for (auto const& it : m_resources)
        { 
            return it;
        }

        return nullptr;
    }

    PipelineNode::Frequency PipelineNode::FrequencyFromString(std::string& _str)
    {
        if (_str == "PER_OBJECT")
            return Frequency::PER_OBJECT;
        else if (_str == "PER_LIGHT")
            return Frequency::PER_LIGHT;
        else if (_str == "PER_FRAME")
            return Frequency::PER_FRAME;

        return Frequency::INVALID;
    }
    PipelineNodeResource::Type PipelineNodeResource::TypeFromString(std::string const& _str)
    {
        if (_str == "RENDER_TARGET")
            return Type::RENDER_TARGET;
        else if (_str == "REFERENCE")
            return Type::REFERENCE;
        else if (_str == "SHADER_RESOURCE")
            return Type::SHADER_RESOURCE;

        return Type::INVALID;
    }
    DXGI_FORMAT PipelineNodeResource::FormatFromString(std::string const& _str)
    {
        if (_str == "D32_FLOAT")
            return DXGI_FORMAT_D32_FLOAT;
        else if (_str == "D24_UNORM_S8_UINT")
            return DXGI_FORMAT_D24_UNORM_S8_UINT;

        return DXGI_FORMAT_UNKNOWN;
    }
    
    Pipeline::Pipeline(InputLayout _inputLayout, std::vector<Buffer> _buffers, std::vector<MaterialInput> _materialInputs, ComPtr<ID3D12PipelineState> _pipeline, ComPtr<ID3D12RootSignature> _rootSignature)
        : m_inputLayout(_inputLayout), 
        m_buffers(_buffers), 
        m_materialInputs(_materialInputs),
        m_pipeline(_pipeline), 
        m_rootSignature(_rootSignature)
    {
        const D3D12_INPUT_LAYOUT_DESC* pInputLayoutDesc = nullptr;
        switch (m_inputLayout)
        {
        case InputLayout::PRIMITIVE:
            pInputLayoutDesc = &PrimitiveVertex::InputLayout;
            break;

        case InputLayout::SKINNED:
            pInputLayoutDesc = &SkinnedVertex::InputLayout;
            break;
        }
        DirectX::RenderTargetState renderTargetState(
            DXGI_FORMAT_D32_FLOAT,
            DXGI_FORMAT_R8G8B8A8_UNORM
        );

        DirectX::EffectPipelineStateDescription desc(
            pInputLayoutDesc,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullClockwise,
            renderTargetState
        );
    }

    Pipeline::Pipeline(GpuDevice& _gpu, std::string _name)
    {
        using namespace nlohmann;
        std::ifstream stream("Assets/Pipelines/" + _name + ".json");
        json data = json::parse(stream);

        m_inputLayout = InputLayout::PRIMITIVE;
        m_name = (data.contains("Name")) ? data["Name"].get<std::string>() : _name;
        if (data.contains("InputLayout"))
        {
            std::string inputLayout = data["InputLayout"].get<std::string>();
            if (inputLayout == "PRIMITIVE")
                m_inputLayout = InputLayout::PRIMITIVE;
            else if (inputLayout == "SKINNED")
                m_inputLayout = InputLayout::SKINNED;
            else if (inputLayout == "FRAME")
                m_inputLayout == InputLayout::FRAME;
        }

        /*
        * ,
        {
            "Name": "MATERIAL",
            "Register": 1,
            "Data": [
                {
                    "Name": "Albedo",
                    "Type": "Float3",
                    "Value": [0.5, 0.5, 0.5]
                }
            ]
        }
        */
        for (auto& buffer : data["Buffers"])
        {
            std::string bufferName = buffer["Name"].get<std::string>();
            if (bufferName == "SCENE")
                m_buffers.push_back(Buffer::SCENE);
            else if (bufferName == "MATERIAL")
            {
                m_buffers.push_back(Buffer::MATERIAL);
                for (auto& it : buffer["Data"])
                {
                    MaterialInput input;
                    input.name = it["Name"].get<std::string>();
                    input.type = MaterialInput::TypeFromString(it["Type"].get<std::string>());
                    switch (input.type)
                    {
                    case MaterialInput::Type::FLOAT:
                        input.value = it.contains("Value") ? it["Value"].get<float>() : 0.0f;
                        break;
                    case MaterialInput::Type::FLOAT2:
                        input.value = 
                            it.contains("Value") ?
                            Vector2{ it["Value"][0].get<float>(), it["Value"][1].get<float>() } : Vector2{0, 0};
                        break;
                    case MaterialInput::Type::FLOAT3:
                        input.value =
                            it.contains("Value") ?
                            Vector3{ 
                            it["Value"][0].get<float>(),
                            it["Value"][1].get<float>(),
                            it["Value"][2].get<float>() } :
                            Vector3{ 0, 0, 0 };
                        break;
                    case MaterialInput::Type::FLOAT4:
                        input.value =
                            it.contains("Value") ?
                            Vector4{
                            it["Value"][0].get<float>(),
                            it["Value"][1].get<float>(),
                            it["Value"][2].get<float>(),
                            it["Value"][3].get<float>() } :
                            Vector4{ 0, 0, 0, 0 };
                        break;
                    }
                    m_materialInputs.push_back(input);
                    m_materialInputsSize += MaterialInput::typeSizes[static_cast<size_t>(input.type)];
                }
            }
            else if (bufferName == "OBJECT")
                m_buffers.push_back(Buffer::OBJECT);
        }

        for (auto& texture : data["Textures"])
        {
            MaterialInput input = MaterialInput{
                    texture["Name"].get<std::string>(),
                    MaterialInput::Type::TEXTURE,
                    Texture{ texture.contains("Value") ? texture["Value"].get<std::string>() : "" }
            };
            if (std::get<Texture>(input.value).name != "")
            {
                Texture& t = std::get<Texture>(input.value);
                _gpu.LoadTexture(t.name, t);
            }
            m_materialInputs.push_back(input);
        }

        
        const D3D12_INPUT_LAYOUT_DESC* pInputLayoutDesc = nullptr;
        switch (m_inputLayout)
        {
        case InputLayout::PRIMITIVE:
            pInputLayoutDesc = &PrimitiveVertex::InputLayout;
            break;

        case InputLayout::SKINNED:
            pInputLayoutDesc = &SkinnedVertex::InputLayout;
            break;
        }
        DirectX::RenderTargetState renderTargetState(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );

        D3D12_BLEND_DESC blend = CommonStates::Opaque;
        if (data.contains("BlendState"))
        {
            std::string blendState = data["BlendState"].get<std::string>();
            if (blendState == "Opaque")
            {
                blend = CommonStates::Opaque;
            }
            else if (blendState == "AlphaBlend")
            {
                blend = CommonStates::AlphaBlend;
            }
            else if (blendState == "Additive")
            {
                blend = CommonStates::Additive;
            }
            else if (blendState == "NonPremultiplied")
            {
                blend = CommonStates::NonPremultiplied;
            }
        }

        D3D12_DEPTH_STENCIL_DESC depth = CommonStates::DepthDefault;
        if (data.contains("DepthStencil"))
        {
            std::string depthState = data["DepthStencil"];
            if (depthState == "None")
            {
                depth = CommonStates::DepthNone;
            }
            else if (depthState == "Default")
            {
                depth = CommonStates::DepthDefault;
            }
            else if (depthState == "Read")
            {
                depth = CommonStates::DepthRead;
            }
            else if (depthState == "ReverseZ")
            {
                depth = CommonStates::DepthReverseZ;
            }
            else if (depthState == "ReadReverseZ")
            {
                depth = CommonStates::DepthReadReverseZ;
            }
        }

        D3D12_RASTERIZER_DESC rasterizer = CommonStates::CullCounterClockwise;
        if (data.contains("Rasterizer"))
        {
            std::string rast = data["Rasterizer"];
            if (rast == "CullNone")
                rasterizer = CommonStates::CullNone;
            else if (rast == "CullClockwise")
                rasterizer = CommonStates::CullClockwise;
            else if (rast == "CullCounterClockwise")
                rasterizer = CommonStates::CullCounterClockwise;
            else if (rast == "Wireframe")
                rasterizer = CommonStates::Wireframe;
        }
        DirectX::EffectPipelineStateDescription desc(
            pInputLayoutDesc,
            blend,
            depth,
            rasterizer,
            renderTargetState
        );

        std::vector<ComPtr<ID3DBlob>> shaders;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline = desc.GetDesc();
        for (auto& shader : data["Shaders"])
        {
            std::string type = shader["Type"].get<std::string>();
            std::string path = shader["Path"].get<std::string>();
            std::string entry = shader["EntryPoint"].get<std::string>();

            ComPtr<ID3DBlob> shader;
            ComPtr<ID3DBlob> error;
#ifdef _DEBUG
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;
#else
            UINT compileFlags = 0;
#endif
            std::string target;
            if (type == "Vertex")
            {
                target = "vs_5_1";
            }
            else if (type == "Pixel")
            {
                target = "ps_5_1";
            }

            HRESULT hr = D3DCompileFromFile(
                ConvertToWideString("Assets/Shaders/" + path).c_str(),
                nullptr, nullptr, entry.c_str(), target.c_str(),
                compileFlags, 0, &shader, &error
            );
            if (FAILED(hr))
            {
                std::cout << "Error Compiling Shader: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
                throw DirectXException(hr);
            }
            shaders.push_back(shader);

            if (type == "Vertex")
            {
                ThrowIfFailed(_gpu->CreateRootSignature(0, 
                    shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
                pipeline.VS = { shader->GetBufferPointer(), shader->GetBufferSize() };
            }
            else if (type == "Pixel")
            {
                pipeline.PS = { shader->GetBufferPointer(), shader->GetBufferSize() };
            }
        }

        ThrowIfFailed(_gpu->CreateGraphicsPipelineState(
            &pipeline,
            IID_PPV_ARGS(&m_pipeline)
        ));
        m_pipeline->SetName(ConvertToWideString(_name).c_str());
    }

    const Pipeline::InputLayout& Pipeline::GetInputLayout() const
    {
        // TODO: insert return statement here
        return m_inputLayout;
    }
    const std::vector<Pipeline::Buffer>& Pipeline::Buffers() const
    {
        // TODO: insert return statement here
        return m_buffers;
    }
    const std::vector<Pipeline::MaterialInput>& Pipeline::MaterialInputs() const
    {
        // TODO: insert return statement here
        return m_materialInputs;
    }
    const size_t Pipeline::MaterialInputsSize() const
    {
        return m_materialInputsSize;
    }
    const std::string& Pipeline::Name() const
    {
        // TODO: insert return statement here
        return m_name;
    }
    void Pipeline::Bind(CommandList& _cmd) const
    {
        _cmd->SetPipelineState(m_pipeline.Get());
        _cmd->SetGraphicsRootSignature(m_rootSignature.Get());
    }
    Pipeline::MaterialInput::Type Pipeline::MaterialInput::TypeFromString(std::string const& _str)
    {
        if (_str == "Float")
            return Type::FLOAT;
        else if (_str == "Float2")
            return Type::FLOAT2;
        else if (_str == "Float3")
            return Type::FLOAT3;
        else if (_str == "Float4")
            return Type::FLOAT4;
        else if (_str == "Texture")
            return Type::TEXTURE;

        return Type::INVALID;
    }
}
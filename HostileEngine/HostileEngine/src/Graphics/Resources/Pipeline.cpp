#include "stdafx.h"
#include "Pipeline.h" 
#include <nlohmann/json.hpp>
#include <directxtk12/BufferHelpers.h>
#include <directxtk12/EffectPipelineStateDescription.h>
#include <directxtk12/RenderTargetState.h>

#include "Material.h"

namespace Hostile
{
    Pipeline::Pipeline(GpuDevice& _device, const std::string& _name)
        : IGraphicsResource(_device, _name)
    {
    }
    const Pipeline::InputLayout& Pipeline::GetInputLayout() const
    {
        // TODO: insert return statement here
        return m_input_layout;
    }

    const std::vector<Pipeline::Buffer>& Pipeline::Buffers() const
    {
        // TODO: insert return statement here
        return m_buffers;
    }

    const MaterialBufferPtr& Pipeline::MaterialBuffer() const
    {
        // TODO: insert return statement here
        return m_material_buffer;
    }

    const std::vector<MaterialTexture>& Pipeline::Textures() const
    {
        // TODO: insert return statement here
        return m_textures;
    }



    void Pipeline::AddInstance(DrawCall& _draw_call)
    {
        for (auto& draw : m_draws)
        {
            if (draw.material == _draw_call.instance.m_material
                && draw.mesh == _draw_call.instance.m_vertex_buffer)
            {
                ShaderObject object{};
                object.world = _draw_call.world;
                object.normalWorld = _draw_call.world;
                object.normalWorld.Transpose();
                object.normalWorld.Invert();
                object.id = _draw_call.instance.m_id;

                draw.instances.push_back(object);
                draw.stencil = _draw_call.instance.m_stencil;
                return;
            }
        }

        DrawBatch batch{};
        batch.material = _draw_call.instance.m_material;
        batch.mesh = _draw_call.instance.m_vertex_buffer;

        ShaderObject object{};
        object.world = _draw_call.world;
        object.normalWorld = _draw_call.world;
        object.normalWorld.Transpose();
        object.normalWorld.Invert();
        object.id = _draw_call.instance.m_id;

        batch.stencil = _draw_call.instance.m_stencil;
        batch.instances.push_back(object);

        m_draws.push_back(batch);
    }

    void Pipeline::Draw(
        CommandList& _cmd,
        GraphicsResource& _constants,
        GraphicsResource& _lights
    )
    {
        _cmd->SetPipelineState(m_pipeline.Get());
        _cmd->SetGraphicsRootSignature(m_root_signature.Get());

        UINT i = 0;
        UINT material_location = -1;
        UINT instance_location = -1;
        for (auto& it : m_buffers)
        {
            switch (it)
            {
            case Buffer::Scene:
                _cmd->SetGraphicsRootConstantBufferView(
                    i, _constants.GpuAddress());
                break;

            case Buffer::Light:
                _cmd->SetGraphicsRootConstantBufferView(
                    i, _lights.GpuAddress());
                break;

            case Buffer::Material:
                material_location = i;
                break;

            case Buffer::Object:
                instance_location = i;
                break;
            }
            i++;
        }
        UINT texture_start = i;

        for (auto& draw : m_draws)
        {
            draw.material->Bind(_cmd);

            _cmd->IASetVertexBuffers(0, 1, &draw.mesh->GetVBV());
            _cmd->IASetIndexBuffer(&draw.mesh->GetIBV());
            for (auto& instance : draw.instances)
            {
                if (instance_location != -1)
                {
                    D3D12_GPU_VIRTUAL_ADDRESS addr
                        = GraphicsMemory::Get()
                        .AllocateConstant<ShaderObject>(instance).GpuAddress();
                    _cmd->SetGraphicsRootConstantBufferView(instance_location,
                        addr);
                }

                _cmd->OMSetStencilRef(draw.stencil);
                _cmd->DrawIndexedInstanced(
                    draw.mesh->Count(),
                    1, 0, 0, 0
                );
            }
        }
        m_draws.clear();
    }

    void Pipeline::DrawInstanced(
        CommandList& _cmd,
        VertexBufferPtr& _mesh,
        GraphicsResource& _constants,
        D3D12_GPU_DESCRIPTOR_HANDLE& _lights,
        UINT _count
    )
    {
        UINT i = 0;
        UINT material_location = -1;
        UINT instance_location = -1;
        for (auto& it : m_buffers)
        {
            switch (it)
            {
            case Buffer::Scene:
                _cmd->SetGraphicsRootConstantBufferView(
                    i, _constants.GpuAddress());
                break;

            case Buffer::Light:
                _cmd->SetGraphicsRootDescriptorTable(
                    i, _lights);
                break;

            case Buffer::Material:
                material_location = i;
                break;

            case Buffer::Object:
                instance_location = i;
                break;
            }
            i++;
        }
        UINT texture_start = i;

        _cmd->IASetVertexBuffers(0, 1, &_mesh->GetVBV());
        _cmd->IASetIndexBuffer(&_mesh->GetIBV());
        _cmd->DrawIndexedInstanced(_mesh->Count(), _count, 0, 0, 0 );
    }

    PipelinePtr Pipeline::Create(GpuDevice& _gpu, std::string _name)
    {
        using namespace nlohmann;
        std::ifstream stream(_name);
        if (!stream.good())
            return nullptr;
        json data = json::parse(stream);
        PipelinePtr pipeline = std::make_shared<Pipeline>(
            _gpu, data["Name"].get<std::string>());
        pipeline->Init(data);
        return pipeline;
    }

    void Pipeline::Init(const nlohmann::json& _data)
    {
        using namespace nlohmann;
        m_input_layout = InputLayout::Primitive;

        if (_data.contains("InputLayout"))
        {
            std::string inputLayout = _data["InputLayout"].get<std::string>();
            if (inputLayout == "PRIMITIVE")
                m_input_layout = InputLayout::Primitive;
            else if (inputLayout == "SKINNED")
                m_input_layout = InputLayout::Skinned;
            else if (inputLayout == "FRAME")
                m_input_layout == InputLayout::Frame;
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
        size_t offset = 0;
        UINT bind_point = 0;
        for (auto& buffer : _data["Buffers"])
        {
            std::string bufferName = buffer["Name"].get<std::string>();
            if (bufferName == "Scene")
                m_buffers.push_back(Buffer::Scene);
            else if (bufferName == "Material")
            {
                m_buffers.push_back(Buffer::Material);
                std::vector<MaterialBuffer::Input> buffer_inputs;
                for (auto& it : buffer["Data"])
                {
                    MaterialBuffer::Input input;
                    input.name = it["Name"].get<std::string>();
                    input.type = MaterialBuffer::TypeFromString(
                        it["Type"].get<std::string>()
                    );
                    input.offset = offset;

                    switch (input.type)
                    {
                    case MaterialBuffer::Type::Float:
                        input.value = it.contains("Value")
                            ? it["Value"].get<float>() : 0.0f;
                        break;
                    case MaterialBuffer::Type::Float2:
                        input.value =
                            it.contains("Value") ?
                            Vector2{
                            it["Value"][0].get<float>(),
                            it["Value"][1].get<float>()
                        } : Vector2{ 0, 0 };

                        break;
                    case MaterialBuffer::Type::Float3:
                        input.value =
                            it.contains("Value") ?
                            Vector3{
                                it["Value"][0].get<float>(),
                                it["Value"][1].get<float>(),
                                it["Value"][2].get<float>() 
                        } :
                            Vector3{ 0, 0, 0 };
                        break;
                    case MaterialBuffer::Type::Float4:
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

                    buffer_inputs.push_back(input);
                }
                m_material_buffer = MaterialBuffer::Create(
                    "MaterialBuffer",
                    bind_point,
                    buffer_inputs
                );
            }
            else if (bufferName == "Object")
                m_buffers.push_back(Buffer::Object);
            else if (bufferName == "Light")
                m_buffers.push_back(Buffer::Light);

            bind_point++;
        }

        
        if (_data.contains("Textures"))
        {
            for (auto& texture : _data["Textures"])
            {
                m_textures.push_back(
                    { texture["Name"].get<std::string>(), bind_point });
                bind_point++;
            }
        }


        const D3D12_INPUT_LAYOUT_DESC* pInputLayoutDesc = nullptr;
        switch (m_input_layout)
        {
        case InputLayout::Primitive:
            pInputLayoutDesc = &PrimitiveVertex::InputLayout;
            break;

        case InputLayout::Skinned:
            pInputLayoutDesc = &SkinnedVertex::InputLayout;
            break;
        }
        DirectX::RenderTargetState renderTargetState(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );

        D3D12_BLEND_DESC blend = CommonStates::Opaque;
        if (_data.contains("BlendState"))
        {
            std::string blendState = _data["BlendState"].get<std::string>();
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

                blend.IndependentBlendEnable = true;
                blend.RenderTarget[1].BlendEnable = false;
                blend.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }
            else if (blendState == "NonPremultiplied")
            {
                blend = CommonStates::NonPremultiplied;
            }
        }

        D3D12_DEPTH_STENCIL_DESC depth = CommonStates::DepthDefault;
        if (_data.contains("DepthStencil"))
        {
            std::string depthState = _data["DepthStencil"];
            if (depthState == "None")
            {
                depth = CommonStates::DepthNone;
                renderTargetState.dsvFormat = DXGI_FORMAT_UNKNOWN;
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
        if (_data.contains("Rasterizer"))
        {
            std::string rast = _data["Rasterizer"];
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
        for (auto& shader : _data["Shaders"])
        {
            std::string type = shader["Type"].get<std::string>();
            std::string path = shader["Path"].get<std::string>();
            std::string entry = shader["EntryPoint"].get<std::string>();

            ComPtr<ID3DBlob> shader;
            ComPtr<ID3DBlob> error;
#ifdef _DEBUG
            UINT compileFlags 
                = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;
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
                std::cout << "Error Compiling Shader: " 
                    << static_cast<char*>(error->GetBufferPointer()) 
                    << std::endl;
                throw DirectXException(hr);
            }
            shaders.push_back(shader);

            if (type == "Vertex")
            {
                ThrowIfFailed(
                    m_device->CreateRootSignature(0, 
                        shader->GetBufferPointer(), 
                        shader->GetBufferSize(), 
                        IID_PPV_ARGS(&m_root_signature)));

                pipeline.pRootSignature = m_root_signature.Get();

                pipeline.VS = { 
                    shader->GetBufferPointer(), shader->GetBufferSize() };
            }
            else if (type == "Pixel")
            {
                pipeline.PS = { 
                    shader->GetBufferPointer(), shader->GetBufferSize() };
            }
        }

        int i = 0;
        for (auto& render_targets : _data["RenderTargets"])
        {
            pipeline.RTVFormats[i] 
                = FormatFromString(render_targets["Format"]);
            i++;
        }
        pipeline.NumRenderTargets = i;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(
            &pipeline,
            IID_PPV_ARGS(&m_pipeline)
        ));
        m_pipeline->SetName(ConvertToWideString(m_name).c_str());
    }
}
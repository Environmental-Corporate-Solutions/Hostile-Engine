#include "stdafx.h"
#include "Material.h"

#include <misc/cpp/imgui_stdlib.h>

namespace Hostile
{
    MaterialImplPtr MaterialImpl::Create(GpuDevice& _device, const std::string& _name)
    {
        std::ifstream stream(_name);
        if (!stream.good())
        {
            return nullptr;
        }

        using namespace nlohmann;
        json data = json::parse(stream);
        MaterialImplPtr material = std::make_shared<MaterialImpl>(
            _device, data["Name"].get<std::string>()
        );

        material->m_path = _name;
        material->Init(data);
        return material;
    }

    void MaterialImpl::SetPipeline(PipelinePtr _pipeline)
    {
        m_pipeline = _pipeline;

        const MaterialBufferPtr& pipeline_buffer =
            m_pipeline->MaterialBuffer();
        if (pipeline_buffer)
            m_material_buffer = pipeline_buffer->Clone();

        for (const auto& texture : m_pipeline->Textures())
        {
            m_textures[texture.name] = nullptr;
            m_material_textures[texture.name] = texture;
        }
    }

    PipelinePtr MaterialImpl::GetPipeline()
    {
        return m_pipeline;
    }

    void MaterialImpl::Bind(CommandList& _cmd)
    {
        if (m_material_buffer)
            m_material_buffer->Bind(_cmd);
        for (auto& [name, texture] : m_textures)
        {
            if (texture)
                texture->Bind(_cmd);
        }
    }

    void MaterialImpl::RenderImGui()
    {

        if (m_material_buffer)
        {

            for (auto& [name, value] : m_material_buffer->GetValues())
            {
                switch (value.type)
                {
                case MaterialBuffer::Type::Float:
                {
                    float val = std::get<float>(value.value);
                    if (ImGui::SliderFloat(name.c_str(), &val, 0.0f, 1.0f))
                    {
                        m_material_buffer->SetValue(name, val);
                    }
                }
                break;
                case MaterialBuffer::Type::Float2:
                {
                    Vector2 val = std::get<Vector2>(value.value);
                    if (ImGui::DragFloat2(name.c_str(), &val.x))
                    {
                        m_material_buffer->SetValue(name, val);
                    }
                }
                break;
                case MaterialBuffer::Type::Float3:
                {
                    Vector3 val = std::get<Vector3>(value.value);
                    if (ImGui::ColorEdit3(name.c_str(), &val.x))
                    {
                        m_material_buffer->SetValue(name, val);
                    }
                }
                break;
                case MaterialBuffer::Type::Float4:
                {
                    Vector4 val = std::get<Vector4>(value.value);
                    if (ImGui::DragFloat4(name.c_str(), &val.x))
                    {
                        m_material_buffer->SetValue(name, val);
                    }
                }
                break;
                }
            }
        }

        for (auto& [name, texture] : m_material_textures)
        {
            ImGui::InputText(name.c_str(), &texture.scratch);
            ImGui::SameLine();
            if (ImGui::Button("Apply"))
            {
                texture.name = texture.scratch;
                m_textures[name] = ResourceLoader::Get()
                    .GetOrLoadResource<Texture>(texture.name);
                m_textures[name]->SetBindPoint(texture.bind_point);
            }
        }
    }
    
    MaterialBufferPtr& MaterialImpl::MaterialBuffer()
    {
        // TODO: insert return statement here
        return m_material_buffer;
    }

    const std::string& MaterialImpl::Path()
    {
        // TODO: insert return statement here
        return m_path;
    }

    void MaterialImpl::Init(const nlohmann::json& _data)
    {
        if (_data.contains("Pipeline"))
        {
            auto pipeline = ResourceLoader::Get()
                .GetOrLoadResource<Pipeline>(_data["Pipeline"]
                    .get<std::string>());
            SetPipeline(pipeline);
        }

        if (_data.contains("Values"))
        {
            for (auto& [name, value] : _data["Values"].items())
            {
                if (value.is_string())
                {
                    m_textures[name] = ResourceLoader::Get()
                        .GetOrLoadResource<Texture>(value.get<std::string>());
                    m_material_textures[name].name = value.get<std::string>();
                    m_material_textures[name].scratch 
                        = value.get<std::string>();
                    m_textures[name]->SetBindPoint(
                        m_material_textures[name].bind_point);
                }
                else
                {
                    if (value.size() == 1)
                    {
                        m_material_buffer->SetValue(
                            name,
                            value.get<float>()
                        );
                    }
                    else if (value.size() == 2)
                    {
                        m_material_buffer->SetValue(
                            name,
                            Vector2{ 
                                value[0].get<float>(), 
                                value[1].get<float>() }
                        );
                    }
                    else if (value.size() == 3)
                    {
                        Vector3 val = ReadVec3(value);
                        m_material_buffer->SetValue(
                            name,
                            val
                        );
                    }
                    else if (value.size() == 4)
                    {
                        Vector4 val = ReadVec4(value);
                        m_material_buffer->SetValue(
                            name,
                            val
                        );
                    }
                }
            }
        }
    }
}
#include <stdafx.h>
#include "MaterialBuffer.h"

namespace Hostile
{
    MaterialBuffer::InputMap& MaterialBuffer::GetValues()
    {
        return m_inputs;
    }

    void MaterialBuffer::Bind(CommandList& _cmd)
    {
        _cmd->SetGraphicsRootConstantBufferView(
            m_bind_point,
            m_resource.GpuAddress()
        );
    }

    MaterialBufferPtr MaterialBuffer::Clone()
    {
        std::unique_ptr<MaterialBuffer> material_buffer =
            std::make_unique<MaterialBuffer>(m_name, m_bind_point);
        material_buffer->m_size = m_size;
        material_buffer->m_inputs = m_inputs;
        material_buffer->m_resource = GraphicsMemory::Get().Allocate(m_size);
        return material_buffer;
    }
    
    MaterialBuffer::Type MaterialBuffer::TypeFromString(
        const std::string& _str
    )
    {
        if (_str == "Float")
            return Type::Float;
        else if (_str == "Float2")
            return Type::Float2;
        else if (_str == "Float3")
            return Type::Float3;
        else if (_str == "Float4")
            return Type::Float4;
    }

    MaterialBufferPtr MaterialBuffer::Create(
        const std::string& _name,
        UINT _bind_point,
        std::vector<Input>& _inputs
    )
    {
        std::unique_ptr<MaterialBuffer> material_buffer
            = std::make_unique<MaterialBuffer>(_name, _bind_point);
        material_buffer->Init(_inputs);
        return material_buffer;
    }

    void MaterialBuffer::Init(std::vector<Input>& _inputs)
    {
        UINT offset = 0;
        for (auto& input : _inputs)
        {
            input.offset = offset;
            m_inputs[input.name] = input;
            offset += type_sizes[static_cast<UINT>(input.type)];
        }
        m_size = (offset + 255) & ~255;
        m_resource = GraphicsMemory::Get().Allocate(m_size, 256Ui64);
    }
}
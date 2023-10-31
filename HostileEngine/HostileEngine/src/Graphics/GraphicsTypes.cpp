#include "stdafx.h"
#include "GraphicsTypes.h"
#include "DirectPipeline.h"

namespace Hostile
{
    void Material::SetPipeline(PipelinePtr _pipeline)
    {
        m_material_inputs = _pipeline->MaterialInputs();
        m_size = _pipeline->MaterialInputsSize();
        m_pipeline = _pipeline;
        
        UpdateValues();
    }

    void Material::UpdateValues()
    {
        if (m_size)
        {
            m_resource = GraphicsMemory::Get().Allocate(m_size, 256);
            UINT8* data = (UINT8*)m_resource.Memory();
            for (auto& it : m_material_inputs)
            {
                switch (it.type)
                {
                case MaterialInput::Type::FLOAT:
                    *(float*)data = std::get<float>(it.value);
                    data += sizeof(float);
                    break;

                case MaterialInput::Type::FLOAT2:
                    *(Vector2*)data = std::get<Vector2>(it.value);
                    data += sizeof(Vector2);
                    break;

                case MaterialInput::Type::FLOAT3:
                    *(Vector3*)data = std::get<Vector3>(it.value);
                    data += sizeof(Vector3);
                    break;

                case MaterialInput::Type::FLOAT4:
                    *(Vector4*)data = std::get<Vector4>(it.value);
                    data += sizeof(Vector4);
                    break;
                }
            }
        }
    }
}
#include "stdafx.h"
#include "ReadBackBuffer.h"

namespace Hostile
{
    bool ReadBackBuffer::Map(UINT8** _pdata)
    {
        if (FAILED(m_readback_buffer[m_frame_index]->Map(0, nullptr, reinterpret_cast<void**>(_pdata))))
        {
            return false;
        }
        return true;
    }

    void ReadBackBuffer::SetFrameIndex(UINT _frame_index)
    {
        m_frame_index = _frame_index;
    }

    ComPtr<ID3D12Resource> ReadBackBuffer::GetResource()
    {
        return m_readback_buffer[m_frame_index];
    }

    ReadBackBufferPtr ReadBackBuffer::Create(GpuDevice& _device, ReadBackBufferCreateInfo& _create_info)
    {
        ReadBackBufferPtr buffer = std::make_shared<ReadBackBuffer>();
        buffer->Init(_device, _create_info);
        return buffer;
    }

    void ReadBackBuffer::Init(GpuDevice& _device, ReadBackBufferCreateInfo& _create_info)
    {
        CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Buffer(
            _create_info.dimensions.x * _create_info.dimensions.y * sizeof(float));
        for (size_t i = 0; i < g_frame_count; i++)
        {
            ThrowIfFailed(_device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_readback_buffer[i])));
        }
    }
}
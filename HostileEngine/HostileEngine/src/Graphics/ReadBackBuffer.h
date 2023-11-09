#pragma once
#include "IReadBackBuffer.h"
#include "GraphicsHelpers.h"
#include "GpuDevice.h"

namespace Hostile
{
    class ReadBackBuffer;
    using ReadBackBufferPtr = std::shared_ptr<ReadBackBuffer>;

    class ReadBackBuffer : public IReadBackBuffer
    {
    public:
        bool Map(UINT8** _pdata);
        void SetFrameIndex(UINT _frame_index);
        ComPtr<ID3D12Resource> GetResource();

        struct ReadBackBufferCreateInfo
        {
            Vector2 dimensions;
            DXGI_FORMAT format;
        };
        static ReadBackBufferPtr Create(GpuDevice& _device, ReadBackBufferCreateInfo& _create_info);
    private:
        void Init(GpuDevice& _device, ReadBackBufferCreateInfo& _create_info);
        std::array<ComPtr<ID3D12Resource>, g_frame_count> m_readback_buffer;
        UINT m_frame_index = 0;
    };
}


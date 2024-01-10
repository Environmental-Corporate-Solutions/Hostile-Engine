#pragma once
#include "../ResourceLoader.h"

namespace Hostile
{
    class VertexBuffer;
    using VertexBufferPtr = std::shared_ptr<VertexBuffer>;
    class VertexBuffer : public IGraphicsResource
    {
    public:
        static constexpr TypeID type_id = 3;
        static TypeID TypeID() { return type_id; }
        static VertexBufferPtr Create(
            GpuDevice& _device, const std::string& _name);

        D3D12_VERTEX_BUFFER_VIEW GetVBV();
        D3D12_INDEX_BUFFER_VIEW GetIBV();
        UINT Count();

        VertexBuffer(GpuDevice& _device, const std::string& _name)
            : IGraphicsResource(_device, _name) {}
    private:
        void Init();
        

        D3D12ResourcePtr m_vertex_buffer = nullptr;
        D3D12ResourcePtr m_index_buffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_vbv{};
        D3D12_INDEX_BUFFER_VIEW m_ibv{};

        UINT m_index_count = 0;
    };
}


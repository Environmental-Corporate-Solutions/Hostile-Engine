#pragma once
#include "../ResourceLoader.h"

namespace Hostile 
{
    class Texture;
    using TexturePtr = std::shared_ptr<Texture>;
    class Texture : public IGraphicsResource
    {
    public:
        static constexpr TypeID type_id = 2;
        static TypeID TypeID() { return type_id; }
        static TexturePtr Create(GpuDevice& _device, const std::string& _name);

        void SetBindPoint(UINT _bind_point);
        void Bind(CommandList& _cmd);

        Texture(GpuDevice& _device, const std::string& _name)
            : IGraphicsResource(_device, _name) {}
    private:
        void Init(const std::string& _path);
        

        ComPtr<ID3D12Resource> m_texture = nullptr;
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle = { UINT64(-1) };
        UINT m_resource_heap_index = UINT(-1);

        UINT m_bind_point = UINT(-1);
    };
}


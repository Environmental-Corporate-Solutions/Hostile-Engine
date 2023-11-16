#pragma once
#include "IRenderTarget.h"
#include <directxtk12/DescriptorHeap.h>
#include "GraphicsHelpers.h"
#include "GpuDevice.h"

#include "ReadBackBuffer.h"

namespace Hostile
{
    class RenderTarget;
    using RenderTargetPtr = std::shared_ptr<RenderTarget>;
    class RenderTarget : public IRenderTarget
    {
    public:
        explicit RenderTarget(GpuDevice& _device, DXGI_FORMAT _format, Vector2 _dimensions);
        ~RenderTarget() final = default;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const;
        ComPtr<ID3D12Resource>& GetTexture();

        void IncrementFrameIndex();

        void Clear(CommandList& _cmd);
        void Submit(CommandList& _cmd);

        D3D12_VIEWPORT GetViewport() const;
        D3D12_RECT GetScissor() const;

        //-----------------------------------------------------------
        // VIRTUAL
        //-----------------------------------------------------------
        Vector2 GetDimensions() final;
        void SetDimensions(const Vector2& _dimensions) final;
        UINT64 GetPtr() final;

        void BindReadBackBuffer(IReadBackBufferPtr _readback_buffer);


        struct RenderTargetCreateInfo
        {
            Vector2 dimensions = {};
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            bool placed = false;
            ComPtr<ID3D12Heap> heap = nullptr;
            UINT offset = 0;
            CD3DX12_RESOURCE_DESC resource_desc;
        };
        static RenderTargetPtr Create(GpuDevice& _device, RenderTargetCreateInfo& _create_info);
    private:
        void Init(GpuDevice& _device, RenderTargetCreateInfo& _create_info);
        void InitPlaced(GpuDevice& _device, RenderTargetCreateInfo& _create_info);
        GpuDevice& m_device;

        std::array<ComPtr<ID3D12Resource>, g_frame_count>      m_texture{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, g_frame_count> m_rtv{};
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, g_frame_count> m_srv{};
        std::array<UINT64, g_frame_count>                      m_srv_indices{};
        std::unique_ptr<DirectX::DescriptorHeap>               m_heap{};

        Vector2 m_extent{};
        size_t         m_frame_index = 0;
        D3D12_VIEWPORT m_vp{};
        D3D12_RECT     m_scissor{};

        D3D12_CLEAR_VALUE m_clear_value{};

        ReadBackBufferPtr m_readback_buffer;
    };
    
}


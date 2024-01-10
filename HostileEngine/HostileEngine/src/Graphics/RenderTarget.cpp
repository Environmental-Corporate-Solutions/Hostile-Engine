#include "stdafx.h"
#include "RenderTarget.h"

#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/BufferHelpers.h>


namespace Hostile
{
    RenderTarget::RenderTarget(GpuDevice& _device, DXGI_FORMAT _format, Vector2 _dimensions)
        : m_device(_device), m_vp({ 0, 0, _dimensions.x, _dimensions.y, 0, 1 }),
        m_scissor(D3D12_RECT{ 0, 0, (long)_dimensions.x, (long)_dimensions.y }),
        m_clear_value({ _format, { 0, 0, 0, 1 } })
    {
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRTV() const
    {
        return m_rtv[m_frame_index];
    }

    D3D12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetSRV() const
    {
        return m_srv[m_frame_index];
    }

    ComPtr<ID3D12Resource>& RenderTarget::GetTexture()
    {
        return m_texture[m_frame_index];
    }

    void RenderTarget::IncrementFrameIndex()
    {
        m_frame_index++;
        m_frame_index %= g_frame_count;
        if (m_readback_buffer)
            m_readback_buffer->SetFrameIndex(m_frame_index);
    }

    void RenderTarget::Clear(CommandList& _cmd)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_texture[m_frame_index].Get(),
            D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        _cmd->ResourceBarrier(1, &barrier);
        _cmd->ClearRenderTargetView(m_rtv[m_frame_index], m_clear_value.Color, 0, nullptr);
    }

    void RenderTarget::Submit(CommandList& _cmd)
    {
        if (m_readback_buffer)
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_texture[m_frame_index].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_COPY_SOURCE
            );
            _cmd->ResourceBarrier(1, &barrier);

            //D3D12_TEXTURE_COPY_LOCATION loc{};
            //loc.pResource = m_texture[m_frame_index].Get();
            //loc.SubresourceIndex = 0;
            //loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;


            //D3D12_TEXTURE_COPY_LOCATION loc2{};
            //loc2.pResource = m_texture[m_frame_index].Get();
            //loc2.SubresourceIndex = 0;
            //loc2.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            UINT64 row_pitch = 0;
            UINT row_count = 0;
            ComPtr<ID3D12Device> device;
            UINT64 total_resource_size = 0;
            _cmd->GetDevice(IID_PPV_ARGS(&device));
            device->GetCopyableFootprints(
                &m_texture[m_frame_index]->GetDesc(),
                0, 1, 0,
                nullptr,
                &row_count,
                &row_pitch,
                &total_resource_size
            );

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
            bufferFootprint.Footprint.Width = static_cast<UINT>(m_vp.Width);
            bufferFootprint.Footprint.Height = static_cast<UINT>(m_vp.Height);
            bufferFootprint.Footprint.Depth = 1;
            bufferFootprint.Footprint.RowPitch = static_cast<UINT>((row_pitch + 255) & ~0xFFu);
            bufferFootprint.Footprint.Format = m_clear_value.Format;

            const CD3DX12_TEXTURE_COPY_LOCATION copyDest(m_readback_buffer->GetResource().Get(), bufferFootprint);
            const CD3DX12_TEXTURE_COPY_LOCATION copySrc(m_texture[m_frame_index].Get(), 0);
            _cmd->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);
            //_cmd->CopyBufferRegion(m_readback_buffer->GetResource().Get(), 0, m_texture[m_frame_index].Get(), 0, m_vp.Width * m_vp.Height * sizeof(float));

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_texture[m_frame_index].Get(),
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            );
            _cmd->ResourceBarrier(1, &barrier);
        }
        else 
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_texture[m_frame_index].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
            );
            _cmd->ResourceBarrier(1, &barrier);
        }
    }

    D3D12_VIEWPORT RenderTarget::GetViewport() const
    {
        return m_vp;
    }
    

    D3D12_RECT RenderTarget::GetScissor() const
    {
        return m_scissor;
    }

    Vector2 RenderTarget::GetDimensions()
    {
        return { m_vp.Width, m_vp.Height };
    }

#undef min
    void RenderTarget::SetDimensions(const Vector2& _dimensions)
    {
        m_vp.Width  = std::min(_dimensions.x, m_extent.x);
        m_vp.Height = std::min(_dimensions.y, m_extent.y);
        m_scissor.right  = m_vp.Width;
        m_scissor.bottom = m_vp.Height;
    }

    UINT64 RenderTarget::GetPtr()
    {
        return m_srv[(m_frame_index + 0) % g_frame_count].ptr;
    }

    void RenderTarget::BindReadBackBuffer(IReadBackBufferPtr _readback_buffer)
    {
        m_readback_buffer = std::static_pointer_cast<ReadBackBuffer>(_readback_buffer);
    }

    RenderTargetPtr RenderTarget::Create(GpuDevice& _device, RenderTargetCreateInfo& _create_info)
    {
        RenderTargetPtr render_target = std::make_shared<RenderTarget>(_device, _create_info.format, _create_info.dimensions);
        if (render_target)
        {
            if (!_create_info.placed)
                render_target->Init(_device, _create_info);
            else
                render_target->InitPlaced(_device, _create_info);
            return render_target;
        }
        return nullptr;
    }

    void RenderTarget::Init(GpuDevice& _device, RenderTargetCreateInfo& _create_info)
    {
        using namespace DirectX;
        m_heap = std::make_unique<DescriptorHeap>(
            _device.Device().Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            g_frame_count
        );

        m_extent = _create_info.dimensions;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            _create_info.format,
            (UINT64)_create_info.dimensions.x,
            (UINT)_create_info.dimensions.y
        );
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        for (UINT64 i = 0; i < g_frame_count; i++)
        {
            ThrowIfFailed(_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &m_clear_value,
                IID_PPV_ARGS(&m_texture[i])
            ));

            m_srv_indices[i] = _device.ResourceHeap().Allocate();
            m_srv[i] = _device.ResourceHeap().GetGpuHandle(m_srv_indices[i]);
            m_rtv[i] = m_heap->GetCpuHandle(i);
            _device->CreateRenderTargetView(m_texture[i].Get(), nullptr, m_heap->GetCpuHandle(i));
            _device->CreateShaderResourceView(m_texture[i].Get(), nullptr, _device.ResourceHeap().GetCpuHandle(m_srv_indices[i]));
        }
    }

    void RenderTarget::InitPlaced(GpuDevice& _device, RenderTargetCreateInfo& _create_info)
    {
        using namespace DirectX;
        m_heap = std::make_unique<DescriptorHeap>(
            _device.Device().Get(),
            (_create_info.resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) 
            == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? D3D12_DESCRIPTOR_HEAP_TYPE_RTV : D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            g_frame_count
        );
        if ((_create_info.resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            m_clear_value.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ 0.0f, 0 };
        m_extent = _create_info.dimensions;

        for (UINT64 i = 0; i < g_frame_count; i++)
        {
            ThrowIfFailed(_device->CreatePlacedResource(
                _create_info.heap.Get(),
                _create_info.offset,
                &_create_info.resource_desc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &m_clear_value,
                IID_PPV_ARGS(&m_texture[i])
            ));

            m_srv_indices[i] = _device.ResourceHeap().Allocate();
            m_srv[i] = _device.ResourceHeap().GetGpuHandle(m_srv_indices[i]);
            m_rtv[i] = m_heap->GetCpuHandle(i);
            if ((_create_info.resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
                == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
            {
                _device->CreateRenderTargetView(m_texture[i].Get(), nullptr, m_heap->GetCpuHandle(i));
                _device->CreateShaderResourceView(m_texture[i].Get(), nullptr, _device.ResourceHeap().GetCpuHandle(m_srv_indices[i]));
            } 
            else if ((_create_info.resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
                == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                dsv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipLevels = 1;
                dsv_desc.Texture2D.MostDetailedMip = 0;
                dsv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                

                _device->CreateDepthStencilView(m_texture[i].Get(), nullptr, m_heap->GetCpuHandle(i));
                _device->CreateShaderResourceView(m_texture[i].Get(), &dsv_desc, _device.ResourceHeap().GetCpuHandle(m_srv_indices[i]));
            }
        }
    }
}
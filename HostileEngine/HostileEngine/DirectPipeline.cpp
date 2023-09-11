#include "stdafx.h"
#include "DirectPipeline.h"

HRESULT DirectPipeline::Init(
    ComPtr<ID3D12Device>& _device,
    DXGI_FORMAT _rtvFormat,
    D3D12_VIEWPORT _viewport
)
{
    HRESULT hr = S_OK;
    m_viewport = _viewport;
    auto desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthEnable = true;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    RIF(m_pipeline.Init(_device, desc), "Failed to Create Pipeline");

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = FRAME_COUNT;
    RIF(
        _device->CreateDescriptorHeap(
            &rtvDesc,
            IID_PPV_ARGS(&m_rtvHeap)
        ),
        "Failed to Create RTV heap"
    );

    m_rtvHeapIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = FRAME_COUNT + 1;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    RIF(
        _device->CreateDescriptorHeap(
            &srvDesc,
            IID_PPV_ARGS(&m_srvHeap)
        ),
        "Failed to Create RTV heap"
    );

    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    RIF(
        _device->CreateDescriptorHeap(
            &rtvDesc,
            IID_PPV_ARGS(&m_dsvHeap)
        ),
        "Failed to create DSV Heap"
    );

    m_srvHeapIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_dsvHeapIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    for (int i = 0; i < FRAME_COUNT; i++)
    {
        RIF(m_cmds[i].Init(_device), "Failed to Create Command List");

        CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(_rtvFormat, m_viewport.Width, m_viewport.Height);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clearValue{ _rtvFormat, { 0.3411f, 0.2117f, 0.0196f, 1 } };
        RIF(
            _device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &texDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&m_rtvs[i])
            ),
            "Failed to Create RTV Texture"
        );
        m_rtvs[i]->SetName((std::wstring(L"Direct Pipeline Render Target View") + std::to_wstring(i)).c_str());

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), i + 1, m_srvHeapIncrementSize);
        _device->CreateShaderResourceView(
            m_rtvs[i].Get(),
            nullptr,
            srvHandle
        );

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_rtvHeapIncrementSize);
        _device->CreateRenderTargetView(
            m_rtvs[i].Get(),
            nullptr,
            rtvHandle
        );

        texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_viewport.Width, m_viewport.Height);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1;
        _device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_dsv[i])
        );
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_dsvHeapIncrementSize);
        _device->CreateDepthStencilView(
            m_dsv[i].Get(),
            nullptr,
            dsvHandle
        );
    }

    m_rtvFormat = _rtvFormat;

    return hr;
}

void DirectPipeline::SetFrameIndex(size_t _frameIndex)
{
    m_frameIndex = _frameIndex;
}

HRESULT DirectPipeline::Reset()
{
    HRESULT hr = S_OK;

    CommandList& cmd = m_cmds[m_frameIndex];
    cmd.Wait();

    RIF(cmd.Reset(m_pipeline.m_pipeline), "Failed to Reset CMD");

    auto barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmd->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvHeapIncrementSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_dsvHeapIncrementSize);
    cmd->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    FLOAT color[4] = { 0.3411f, 0.2117f, 0.0196f, 1 };
    cmd->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
    cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
    cmd->SetGraphicsRootSignature(m_pipeline.m_rootSig.Get());
    cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->RSSetViewports(1, &m_viewport);
    D3D12_RECT scissorRect{};
    scissorRect.left = 0;
    scissorRect.right = m_viewport.Width;
    scissorRect.top = 0;
    scissorRect.bottom = m_viewport.Height;
    cmd->RSSetScissorRects(1, &scissorRect);

    return hr;
}

HRESULT DirectPipeline::Resize(UINT _width, UINT _height)
{
    m_viewport.Width = _width;
    m_viewport.Height = _height;
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        CommandList& cmd = m_cmds[i];
            cmd.Wait();
    }
    ComPtr<ID3D12Device> device;
    m_dsvHeap->GetDevice(IID_PPV_ARGS(&device));
    HRESULT hr = S_OK;
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_rtvFormat, m_viewport.Width, m_viewport.Height);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clearValue{ m_rtvFormat, { 0.3411f, 0.2117f, 0.0196f, 1 } };
        RIF(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &texDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&m_rtvs[i])
            ),
            "Failed to Create RTV Texture"
        );
        m_rtvs[i]->SetName((std::wstring(L"Direct Pipeline Render Target View") + std::to_wstring(i)).c_str());

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), i + 1, m_srvHeapIncrementSize);
        device->CreateShaderResourceView(
            m_rtvs[i].Get(),
            nullptr,
            srvHandle
        );

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_rtvHeapIncrementSize);
        device->CreateRenderTargetView(
            m_rtvs[i].Get(),
            nullptr,
            rtvHandle
        );

        texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_viewport.Width, m_viewport.Height);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1;
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_dsv[i])
        );
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_dsvHeapIncrementSize);
        device->CreateDepthStencilView(
            m_dsv[i].Get(),
            nullptr,
            dsvHandle
        );
    }

    return hr;
}

HRESULT DirectPipeline::Execute(ComPtr<ID3D12CommandQueue>& _cmdQueue)
{
    HRESULT hr = S_OK;
    CommandList& cmd = m_cmds[m_frameIndex];
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd->Close();

    ID3D12CommandList* lists[] = { *cmd };
    _cmdQueue->ExecuteCommandLists(_countof(lists), lists);
    RIF(_cmdQueue->Signal(cmd.m_fence.Get(), ++cmd.m_fenceValue), "Failed to Signal");
    RIF(_cmdQueue->Wait(cmd.m_fence.Get(), cmd.m_fenceValue), "Failed to Wait");

    return hr;
}

ComPtr<ID3D12DescriptorHeap> DirectPipeline::GetSRVHeap()
{
    return m_srvHeap;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectPipeline::GetSRV()
{
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), m_frameIndex + 1, m_srvHeapIncrementSize);
    return srvHandle;
}
CommandList& DirectPipeline::GetCmd(size_t _frameIndex)
{
    return m_cmds[_frameIndex];
}
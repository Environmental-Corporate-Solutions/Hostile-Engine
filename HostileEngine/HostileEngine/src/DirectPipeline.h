#pragma once
#include "GraphicsHelpers.h"


class DirectPipeline
{
public:
    HRESULT Init(
        ComPtr<ID3D12Device>& _device,
        DXGI_FORMAT _rtvFormat,
        D3D12_VIEWPORT _viewport
    );

    void SetFrameIndex(size_t _frameIndex);
    HRESULT Reset();
    HRESULT Execute(ComPtr<ID3D12CommandQueue>& _cmdQueue);
    HRESULT Resize(UINT _width, UINT _height);

    ComPtr<ID3D12DescriptorHeap> GetSRVHeap();
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRV();
    CommandList& GetCmd(size_t _frameIndex);
    
private:
    Pipeline m_pipeline;

    CommandList m_cmds[FRAME_COUNT];
    DXGI_FORMAT m_rtvFormat;
    ComPtr<ID3D12Resource> m_rtvs[FRAME_COUNT];
    ComPtr<ID3D12Resource> m_dsv[FRAME_COUNT];

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap; 
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    UINT m_rtvHeapIncrementSize;
    UINT m_srvHeapIncrementSize;
    UINT m_dsvHeapIncrementSize;

    size_t m_frameIndex;
    D3D12_VIEWPORT m_viewport;
};
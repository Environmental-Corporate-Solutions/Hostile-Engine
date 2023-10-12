#pragma once
#include "GraphicsHelpers.h"

using namespace Hostile;

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

    CommandList& GetCmd(size_t _frameIndex);
    
    ComPtr<ID3D12Resource>& GetBuffer(size_t _frameIndex);

    D3D12_VIEWPORT GetViewport() { return m_viewport; }

    Pipeline m_pipeline;

    CommandList m_cmds[FRAME_COUNT];
    DXGI_FORMAT m_rtvFormat;
    ComPtr<ID3D12Resource> m_rtvs[FRAME_COUNT];
    ComPtr<ID3D12Resource> m_dsv[FRAME_COUNT];

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    UINT m_rtvHeapIncrementSize;
    UINT m_dsvHeapIncrementSize;

    size_t m_frameIndex;
    D3D12_VIEWPORT m_viewport;
};
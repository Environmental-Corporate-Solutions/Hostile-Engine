#include "Graphics.h"
#include <codecvt>
#include "backends/imgui_impl_dx12.h"


Graphics::GRESULT Graphics::Init(GLFWwindow* _pWindow)
{
    ComPtr<ID3D12Debug1> debug;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    debug->EnableDebugLayer();
    debug->SetEnableGPUBasedValidation(true);
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(FindAdapter(adapter)))
        return Graphics::G_FAIL;

    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
        return Graphics::G_FAIL;

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
    m_cmdQueue->SetName(L"Command Queue");
    HWND hwnd = glfwGetWin32Window(_pWindow);
    m_hwnd = hwnd;
    if (FAILED(m_swapChain.Init(m_device, adapter, m_cmdQueue, _pWindow)))
        return Graphics::G_FAIL;

    if (FAILED(m_pipeline.Init(m_device)))
        return Graphics::G_FAIL;

    for (int i = 0; i < FRAME_COUNT; i++)
    {
        m_cmds[i].Init(m_device);
        m_texCmds[i].Init(m_device);
    }

    m_loadCmd.Init(m_device);
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = FRAME_COUNT + 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   m_device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(&m_imGuiDescriptorHeap)
    );

   m_device->CreateDescriptorHeap(
       &heapDesc,
       IID_PPV_ARGS(&m_texSrvHeap)
   );

   heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
   heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
   HRESULT hr = m_device->CreateDescriptorHeap(
       &heapDesc,
       IID_PPV_ARGS(&m_texRtvHeap)
   );
   m_texHeapIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   m_texHeapRtvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   ImGui_ImplDX12_Init(
       m_device.Get(),
       FRAME_COUNT,
       m_swapChain.m_format,
       m_texSrvHeap.Get(),
       m_texSrvHeap->GetCPUDescriptorHandleForHeapStart(),
       m_texSrvHeap->GetGPUDescriptorHandleForHeapStart()
   );
   for (int i = 0; i < FRAME_COUNT; i++)
   {
       CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
       CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_swapChain.m_format, m_swapChain.m_viewport.Width, m_swapChain.m_viewport.Height);
       texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        
       D3D12_CLEAR_VALUE clearValue;
       clearValue.Format = m_swapChain.m_format;
       clearValue.Color[0] = 0.3411f;
       clearValue.Color[1] = 0.2117f;
       clearValue.Color[2] = 0.0196f;
       clearValue.Color[3] = 1;
       m_device->CreateCommittedResource(
           &heapProps,
           D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
           &texDesc,
           D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
           &clearValue,
           IID_PPV_ARGS(&m_texRTVs[i])
       );

       m_texRTVs[i]->SetName((std::wstring(L"Texture RTV ") + std::to_wstring(i)).c_str());
       CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_texSrvHeap->GetCPUDescriptorHandleForHeapStart(), i + 1, m_texHeapIncrementSize);
       
       m_device->CreateShaderResourceView(
           m_texRTVs[i].Get(),
           nullptr,
           srvHandle
       );

       CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_texRtvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_texHeapRtvIncrementSize);
       m_device->CreateRenderTargetView(
           m_texRTVs[i].Get(),
           nullptr,
           rtvHandle
       );
   }

    

    m_frameIndex = 0;
    return GRESULT::G_OK;
}

Graphics::GRESULT Graphics::CreateVertexBuffer(std::vector<Vertex> _vertices, VertexBuffer& _vertexBuffer)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    size_t vertexBufferSize = sizeof(Vertex) * _vertices.size();
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    ComPtr<ID3D12Resource> intermediateBuffer;
    if (FAILED(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediateBuffer)
    )))
    {
        return G_FAIL;
    }

    ComPtr<ID3D12Resource> vertexBuffer;
    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)
    )))
    {
        return G_FAIL;
    }

    D3D12_RANGE readRange{ 0,0 };
    UINT8* data;
    intermediateBuffer->Map(0, &readRange, (void**)&data);
    memcpy(data, _vertices.data(), vertexBufferSize);
    intermediateBuffer->Unmap(0, nullptr);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        vertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
    m_loadCmd.Reset(nullptr);
    m_loadCmd->CopyBufferRegion(vertexBuffer.Get(), 0, intermediateBuffer.Get(), 0, vertexBufferSize);
    m_loadCmd->ResourceBarrier(1, &barrier);
    m_loadCmd->Close();
    ID3D12CommandList* lists[] = { *m_loadCmd };
    m_cmdQueue->ExecuteCommandLists(_countof(lists), lists);
    m_cmdQueue->Signal(m_loadCmd.m_fence.Get(), ++m_loadCmd.m_fenceValue);
    m_loadCmd.Wait();

    _vertexBuffer.vertexBuffer = vertexBuffer;
    _vertexBuffer.vertexCount = _vertices.size();
    _vertexBuffer.vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    _vertexBuffer.vbView.SizeInBytes = vertexBufferSize;
    _vertexBuffer.vbView.StrideInBytes = sizeof(Vertex);
    return GRESULT::G_OK;
}

HRESULT Graphics::FindAdapter(ComPtr<IDXGIAdapter>& _adapter)
{
    HRESULT hr = S_OK;
    ComPtr<IDXGIFactory> factory;
    RIF(CreateDXGIFactory(IID_PPV_ARGS(&factory)), "Failed to Create DXGI Factory");

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    int i = 0;
    ComPtr<IDXGIAdapter> adapter;
    SIZE_T currentBest = 0;
    ComPtr<IDXGIAdapter> bestAdapter;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc{};
        adapter->GetDesc(&desc);
        std::cout << converter.to_bytes(desc.Description) << std::endl;
        std::cout << desc.DedicatedSystemMemory << std::endl;
        std::cout << desc.DedicatedVideoMemory << std::endl;
        std::cout << desc.SharedSystemMemory << std::endl;
        if (desc.DedicatedSystemMemory + desc.DedicatedVideoMemory + desc.SharedSystemMemory > currentBest)
        {
            currentBest = desc.DedicatedSystemMemory + desc.DedicatedVideoMemory + desc.SharedSystemMemory;
            bestAdapter = adapter;
        }
        i++;
    }

    _adapter = bestAdapter;

    return hr;
}

void Graphics::BeginFrame()
{
    CommandList& cmd = m_cmds[m_frameIndex];
    //if (cmd.IsInFlight())
        cmd.Wait();

    cmd.Reset(m_pipeline.m_pipeline);
    m_texCmds[m_frameIndex].Reset(m_pipeline.m_pipeline);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);
    cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
    FLOAT color[4] = { 0.3411f, 0.2117f, 0.0196f, 1 };
   

    CD3DX12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    )
    };
    cmd->ResourceBarrier(_countof(barriers), barriers);
    
    cmd->SetPipelineState(m_pipeline.m_pipeline.Get());
    cmd->SetGraphicsRootSignature(m_pipeline.m_rootSig.Get());
    cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->RSSetViewports(1, &m_swapChain.m_viewport);
    cmd->RSSetScissorRects(1, &m_swapChain.m_scissorRect);

    /*D3D12_VIEWPORT vp{};
    vp.Width = 600;
    vp.Height = 600;
    vp.MaxDepth = 1;
    vp.MinDepth = 0;
    vp.TopLeftX = 400;
    vp.TopLeftY = 400;
    cmd->RSSetViewports(1, &vp);*/

    cmd->ClearRenderTargetView(rtv, color, 0, nullptr);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texRTVs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_texCmds[m_frameIndex]->ResourceBarrier(_countof(barriers), barriers);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_texRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_texHeapRtvIncrementSize);
    m_texCmds[m_frameIndex]->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
    m_texCmds[m_frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_texCmds[m_frameIndex]->SetPipelineState(m_pipeline.m_pipeline.Get());
    m_texCmds[m_frameIndex]->SetGraphicsRootSignature(m_pipeline.m_rootSig.Get());
    m_texCmds[m_frameIndex]->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_texCmds[m_frameIndex]->RSSetViewports(1, &m_swapChain.m_viewport);
    m_texCmds[m_frameIndex]->RSSetScissorRects(1, &m_swapChain.m_scissorRect);
    
    ImGui_ImplDX12_NewFrame();
}

void Graphics::RenderVertexBuffer(VertexBuffer& _vertexBuffer)
{
    m_texCmds[m_frameIndex]->IASetVertexBuffers(0, 1, &_vertexBuffer.vbView);
    m_texCmds[m_frameIndex]->DrawInstanced(_vertexBuffer.vertexCount, 1, 0, 0);
}

void Graphics::RenderImGui()
{
    m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_texSrvHeap.GetAddressOf());
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
}

void Graphics::EndFrame()
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texRTVs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    m_texCmds[m_frameIndex]->ResourceBarrier(1, &barrier);
    m_texCmds[m_frameIndex]->Close();
    ID3D12CommandList* lists[] = { *m_texCmds[m_frameIndex] };
    m_cmdQueue->ExecuteCommandLists(1, lists);
    m_cmdQueue->Signal(m_texCmds[m_frameIndex].m_fence.Get(), ++m_texCmds[m_frameIndex].m_fenceValue);
    m_texCmds[m_frameIndex].Wait();

    //m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_texSrvHeap.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE rtvHandle(m_texSrvHeap->GetGPUDescriptorHandleForHeapStart(), m_frameIndex + 1, m_texHeapIncrementSize);
    ImGui::Begin("View");
    float aspect = m_swapChain.m_viewport.Width / m_swapChain.m_viewport.Height;
    ImGui::Image((ImTextureID)rtvHandle.ptr, ImVec2((float)ImGui::GetWindowHeight() * aspect, (float)ImGui::GetWindowHeight()));
    ImGui::End();
    RenderImGui();

    CommandList& cmd = m_cmds[m_frameIndex];
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd->Close();
    lists[0] = cmd.cmd.Get();
    m_cmdQueue->ExecuteCommandLists(1, lists);

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)lists);
    }

    m_swapChain.swapChain->Present(1, 0);
    m_cmdQueue->Signal(cmd.m_fence.Get(), ++cmd.m_fenceValue);
    m_frameIndex++;
    m_frameIndex %= FRAME_COUNT;
}

void Graphics::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    for (auto& it : m_cmds)
    {
        it.Shutdown();
    }
    m_cmdQueue.Reset();
    m_imGuiDescriptorHeap.Reset();
    m_pipeline.m_pipeline.Reset();
    m_swapChain.rtvHeap.Reset();
    
    for (auto& it : m_swapChain.rtvs)
    {
        it.Reset();
    }
    m_swapChain.swapChain.Reset();
}
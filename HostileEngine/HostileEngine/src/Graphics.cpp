#include "Graphics.h"
#include <codecvt>
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

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
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = FRAME_COUNT;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   m_device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(&m_imGuiDescriptorHeap)
    );

    ImGui_ImplDX12_Init(
        m_device.Get(), 
        FRAME_COUNT, 
        m_swapChain.m_format, 
        m_imGuiDescriptorHeap.Get(),
        m_imGuiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        m_imGuiDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
    );

    m_frameIndex = 0;
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

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);
    cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
    FLOAT color[4] = { 0, 1, 0, 1 };
   

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmd->ResourceBarrier(1, &barrier);
    
    cmd->SetPipelineState(m_pipeline.m_pipeline.Get());
    cmd->SetGraphicsRootSignature(m_pipeline.m_rootSig.Get());

    cmd->ClearRenderTargetView(rtv, color, 0, nullptr);
    ImGui_ImplDX12_NewFrame();
}

void Graphics::RenderImGui()
{
    m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_imGuiDescriptorHeap.GetAddressOf());
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
}

void Graphics::EndFrame()
{
    CommandList& cmd = m_cmds[m_frameIndex];
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd->Close();
    ID3D12CommandList* lists[] = { cmd.cmd.Get() };
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
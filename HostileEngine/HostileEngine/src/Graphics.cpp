#include "stdafx.h"
#include "Graphics.h"
#include <codecvt>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <DirectXTex.h>

using namespace DirectX;

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

        m_constBufferSize = sizeof(ConstantBuffer) * 1024;
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_constBufferSize);
        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constBuffers[i])
        );
        D3D12_RANGE readRange{ 0, 0 };
        m_constBuffers[i]->Map(0, &readRange, (void**)&m_cbData[i]);
    }

    D3D12_DESCRIPTOR_HEAP_DESC dhDesc{};
    dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    dhDesc.NumDescriptors = 1024;
    dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    m_device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_dHeap));
    m_numDescriptors = 1024;
    m_srvIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_currentDescriptor = 0;

    m_loadCmd.Init(m_device);

    m_directPipeline.Init(m_device, m_swapChain.m_format, m_swapChain.m_viewport);
    m_frameIndex = 0;
    ImGui_ImplDX12_Init(
        m_device.Get(),
        FRAME_COUNT,
        m_swapChain.m_format,
        m_directPipeline.GetSRVHeap().Get(),
        m_directPipeline.GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(),
        m_directPipeline.GetSRVHeap()->GetGPUDescriptorHandleForHeapStart()
    );

    //Matrix R;
    //XMFLOAT3 eye = { 0, 0, -5 };
    //XMFLOAT3 target = { 0, 0, 0 };
    //XMFLOAT3 up = { 0, 1, 0 };
    //const XMVECTOR eyev = XMLoadFloat3(&eye);
    //const XMVECTOR targetv = XMLoadFloat3(&target);
    //const XMVECTOR upv = XMLoadFloat3(&up);
    //XMStoreFloat4x4(&R, XMMatrixLookAtLH(eyev, targetv, upv));
    //Matrix E;
    //XMStoreFloat4x4(&E, XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), 
    //    m_swapChain.m_viewport.Width / m_swapChain.m_viewport.Height
    //    , 0.1f, 1000000.0f));
    //
    //m_camera = R * E;
    m_camera.SetPerspective(45, m_swapChain.m_viewport.Width / m_swapChain.m_viewport.Height, 0.1f, 1000000);
    m_camera.LookAt({ 0, 0, -5 }, { 0, 0, 0 }, { 0, 1, 0 });
    return GRESULT::G_OK;
}

Graphics::GRESULT Graphics::CreateVertexBuffer(std::vector<Vertex> _vertices, std::vector<uint32_t> _indices, VertexBuffer& _vertexBuffer)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    size_t vertexBufferSize = sizeof(Vertex) * _vertices.size();
    size_t indexBufferSize = sizeof(uint32_t) * _indices.size();
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize + indexBufferSize);
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
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
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

    ComPtr<ID3D12Resource> indexBuffer;
    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    if (FAILED(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&indexBuffer)
    )))
    {
        return G_FAIL;
    }

    D3D12_RANGE readRange{ 0,0 };
    UINT8* data;
    intermediateBuffer->Map(0, &readRange, (void**)&data);
    memcpy(data, _vertices.data(), vertexBufferSize);
    memcpy(data + vertexBufferSize, _indices.data(), indexBufferSize);
    intermediateBuffer->Unmap(0, nullptr);

    CD3DX12_RESOURCE_BARRIER barrier[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
            vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_INDEX_BUFFER
        )
    };
    m_loadCmd.Reset(nullptr);
    m_loadCmd->CopyBufferRegion(vertexBuffer.Get(), 0, intermediateBuffer.Get(), 0, vertexBufferSize);
    m_loadCmd->CopyBufferRegion(indexBuffer.Get(), 0, intermediateBuffer.Get(), vertexBufferSize, indexBufferSize);
    m_loadCmd->ResourceBarrier(_countof(barrier), barrier);
    m_loadCmd->Close();
    ID3D12CommandList* lists[] = { *m_loadCmd };
    m_cmdQueue->ExecuteCommandLists(_countof(lists), lists);
    m_cmdQueue->Signal(m_loadCmd.m_fence.Get(), ++m_loadCmd.m_fenceValue);
    m_loadCmd.Wait();

    _vertexBuffer.vertexBuffer = vertexBuffer;
    _vertexBuffer.vertexCount = _indices.size();
    _vertexBuffer.vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    _vertexBuffer.vbView.SizeInBytes = vertexBufferSize;
    _vertexBuffer.vbView.StrideInBytes = sizeof(Vertex);
    _vertexBuffer.indexBuffer = indexBuffer;
    _vertexBuffer.ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    _vertexBuffer.ibView.Format = DXGI_FORMAT_R32_UINT;
    _vertexBuffer.ibView.SizeInBytes = indexBufferSize;
    return GRESULT::G_OK;
}

HRESULT Graphics::CreateTexture(std::string _name, Texture& _texture) 
{
    HRESULT hr = S_OK;
    TexMetadata metaData;
    ScratchImage image;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string path = "textures/" + _name + ".png";

    RIF(
        LoadFromWICFile(
            converter.from_bytes(path).c_str(),
            WIC_FLAGS::WIC_FLAGS_NONE,
            &metaData,
            image
        ),
        "Failed to Load Texture From File"
    );

    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        metaData.format,
        metaData.width,
        metaData.height,
        metaData.arraySize
    );
    CD3DX12_HEAP_PROPERTIES heapDesc(D3D12_HEAP_TYPE_DEFAULT);
    
    ComPtr<ID3D12Resource> texture;
    RIF(
        m_device->CreateCommittedResource(
            &heapDesc,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&texture)
        ),
        "Failed to Create Texture"
    );

    std::vector<D3D12_SUBRESOURCE_DATA> subresources(image.GetImageCount());
    const Image* pImages = image.GetImages();
    for (int i = 0; i < subresources.size(); i++)
    {
        auto& subresource = subresources[i];
        subresource.RowPitch = pImages[i].rowPitch;
        subresource.SlicePitch = pImages[i].slicePitch;
        subresource.pData = pImages[i].pixels;
    }

    ComPtr<ID3D12Resource> intermediate;
    UINT64 requiredSize = GetRequiredIntermediateSize(texture.Get(), 0, subresources.size());
    heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
    RIF(
        m_device->CreateCommittedResource(
            &heapDesc,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediate)
        ),
        "Failed to Create Intermediate Texture"
    );

    m_loadCmd.Wait();
    m_loadCmd.Reset(nullptr);
    UpdateSubresources(*m_loadCmd, texture.Get(), intermediate.Get(), 0, 0, subresources.size(), subresources.data());
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
    );
    m_loadCmd->ResourceBarrier(1, &barrier);
    m_loadCmd->Close();
    ID3D12CommandList* lists[] = { *m_loadCmd };
    m_cmdQueue->ExecuteCommandLists(1, lists);
    m_cmdQueue->Signal(m_loadCmd.m_fence.Get(), ++m_loadCmd.m_fenceValue);
    m_loadCmd.Wait();
    _texture.texture = texture;

    return hr; 
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
    cmd.Wait();

    cmd.Reset(m_pipeline.m_pipeline);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);
    cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
    FLOAT color[4] = { 0.3411f, 0.2117f, 0.0196f, 1 };


    CD3DX12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_swapChain.swapChain->GetCurrentBackBufferIndex()].Get(),
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

    cmd->ClearRenderTargetView(rtv, color, 0, nullptr);

    m_directPipeline.SetFrameIndex(m_frameIndex);
    m_directPipeline.Reset();
    m_directPipeline.GetCmd(m_frameIndex)->SetDescriptorHeaps(1, m_dHeap.GetAddressOf());

    ImGui_ImplDX12_NewFrame();
}

void Graphics::RenderVertexBuffer(VertexBuffer& _vertexBuffer, Texture& _texture, Matrix _model)
{
    auto cmd = m_directPipeline.GetCmd(m_frameIndex);
    *reinterpret_cast<Matrix*>(m_cbData[m_frameIndex] + m_currentOffset) = _model * m_camera.ViewProjection();
    cmd->SetGraphicsRootConstantBufferView(0, m_constBuffers[m_frameIndex]->GetGPUVirtualAddress() + m_currentOffset);
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_dHeap->GetCPUDescriptorHandleForHeapStart(), m_currentDescriptor, m_srvIncrementSize);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    D3D12_RESOURCE_DESC texDesc = _texture.texture->GetDesc();
    srvDesc.Format = texDesc.Format;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_device->CreateShaderResourceView(
        _texture.texture.Get(),
        &srvDesc,
        srvHandle
    );
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(m_dHeap->GetGPUDescriptorHandleForHeapStart(), m_currentDescriptor, m_srvIncrementSize);
    cmd->SetGraphicsRootDescriptorTable(1, srvGpuHandle);
    (++m_currentDescriptor) %= m_numDescriptors;
    m_currentOffset += sizeof(ConstantBuffer);
    cmd->IASetIndexBuffer(&_vertexBuffer.ibView);
    cmd->IASetVertexBuffers(0, 1, &_vertexBuffer.vbView);
    cmd->DrawIndexedInstanced(_vertexBuffer.vertexCount, 1, 0, 0, 0);
}

void Graphics::RenderImGui()
{
    m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_directPipeline.GetSRVHeap().GetAddressOf());
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
}

void Graphics::EndFrame()
{
    m_currentOffset = 0;
    m_directPipeline.Execute(m_cmdQueue);

    ImGui::Begin("View");
    if (ImGui::IsWindowFocused()&&ImGui::IsWindowDocked())
    {
      
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if (dragDelta.x == 0 && dragDelta.y == 0)
        {
            m_currDragDelta = { dragDelta.x, dragDelta.y };
        }
        
        float x = dragDelta.x - m_currDragDelta.x;
        float y = dragDelta.y - m_currDragDelta.y;
        m_camera.Pitch(-y * 0.1f);
        m_currDragDelta = { dragDelta.x, dragDelta.y };
        m_camera.Yaw(-x * 0.1f);
    }
    float aspect = m_swapChain.m_viewport.Width / m_swapChain.m_viewport.Height;
    float inverseAspect = m_swapChain.m_viewport.Height / m_swapChain.m_viewport.Width;
    ImVec2 imageSize(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

    if ((imageSize.y * aspect) > imageSize.x)
    {
        imageSize.y = imageSize.x * inverseAspect;
    }
    else
    {
        imageSize.x = imageSize.y * aspect;
    }
    ImVec2 cursorPos = ImGui::GetWindowSize();
    cursorPos.x = (cursorPos.x - imageSize.x) * 0.5f;
    cursorPos.y = (cursorPos.y - imageSize.y) * 0.5f;
    ImGui::SetCursorPos(cursorPos);
    ImGui::Image(
        (ImTextureID)m_directPipeline.GetSRV().ptr,
        imageSize
    );
    ImGui::End();
    RenderImGui();

    CommandList& cmd = m_cmds[m_frameIndex];
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_swapChain.rtvs[m_swapChain.swapChain->GetCurrentBackBufferIndex()].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    cmd->ResourceBarrier(1, &barrier);
    cmd->Close();
    ID3D12CommandList* lists[] = { *cmd };
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

    if (m_resize)
    {
        for (int i = 0; i < FRAME_COUNT; i++)
        {
            m_cmds[i].Wait();
        }
        m_swapChain.Resize(m_resizeWidth, m_resizeHeight);
        //m_directPipeline.Resize(m_resizeWidth, m_resizeHeight);
    }
}

void Graphics::OnResize(UINT _width, UINT _height)
{
    m_resize = true;
    m_resizeWidth = _width;
    m_resizeHeight = _height;
}

void Graphics::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    for (auto& it : m_cmds)
    {
        it.Shutdown();
    }
    m_cmdQueue.Reset();
    m_pipeline.m_pipeline.Reset();
    m_swapChain.rtvHeap.Reset();

    for (auto& it : m_swapChain.rtvs)
    {
        it.Reset();
    }
    m_swapChain.swapChain.Reset();
}
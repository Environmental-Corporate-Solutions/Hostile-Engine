#include "stdafx.h"
#include "Graphics.h"
#include <codecvt>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <DirectXTex.h>

#include <directxtk12/WICTextureLoader.h>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/BufferHelpers.h>

using namespace DirectX;

namespace Hostile
{
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

        std::cout << "after device creation" << std::endl;
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
        m_cmdQueue->SetName(L"Command Queue");
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        if (FAILED(m_swapChain.Init(m_device, adapter, m_cmdQueue, _pWindow)))
            return Graphics::G_FAIL;

        std::cout << "after swap chain creation" << std::endl;
        if (FAILED(m_pipeline.Read(m_device, "default_no_depth")))
            return Graphics::G_FAIL;
        std::cout << "after pipeline creation" << std::endl;

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

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvDesc.NumDescriptors = FRAME_COUNT + 1;
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        m_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_imGuiHeap));

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
            m_imGuiHeap.Get(),
            m_imGuiHeap->GetCPUDescriptorHandleForHeapStart(),
            m_imGuiHeap->GetGPUDescriptorHandleForHeapStart()
        );

        m_camera.SetPerspective(45, m_swapChain.m_viewport.Width / m_swapChain.m_viewport.Height, 0.1f, 1000000);
        m_camera.LookAt({ 0, 0, 50 }, { 0, 0, 0 }, { 0, 1, 0 });

        m_graphicsMemory = std::make_unique<GraphicsMemory>(m_device.Get());

        m_states = std::make_unique<CommonStates>(m_device.Get());
        m_shape = GeometricPrimitive::CreateSphere();
        ResourceUploadBatch resourceUpload(m_device.Get());
        resourceUpload.Begin();
        m_shape->LoadStaticBuffers(m_device.Get(), resourceUpload);
        auto uploadResourcesFinished = resourceUpload.End(m_cmdQueue.Get());
        uploadResourcesFinished.wait();
        RenderTargetState sceneState(
            m_swapChain.m_format,
            DXGI_FORMAT_D32_FLOAT
        );
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            sceneState
        );
        m_effect = std::make_unique<BasicEffect>(m_device.Get(), EffectFlags::Lighting, pd);
        m_effect->EnableDefaultLighting();

        const D3D12_INPUT_ELEMENT_DESC InputElements[] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",       0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES",0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        D3D12_INPUT_LAYOUT_DESC inputLayout;
        inputLayout.NumElements = _countof(InputElements);
        inputLayout.pInputElementDescs = InputElements;
        EffectPipelineStateDescription skinnedpd(
            &inputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            sceneState
        );

        m_skinnedEffect = std::make_unique<SkinnedEffect>(m_device.Get(), EffectFlags::None, skinnedpd);

        m_resourceDescriptors = std::make_unique<DescriptorHeap>(m_device.Get(), 1024);

        m_samplerDescriptors = std::make_unique<DescriptorHeap>(m_device.Get(), 
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 2);
        D3D12_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        m_device->CreateSampler(&sampler, m_samplerDescriptors->GetCpuHandle(0));

        return GRESULT::G_OK;
    }

    void Graphics::RenderDebug(Matrix& _mat)
    {
        CommandList& cmd = m_directPipeline.GetCmd(m_frameIndex);
        m_effect->SetMatrices(_mat, m_camera.View(), m_camera.Projection());
        m_effect->Apply(*cmd);
        m_shape->Draw(*cmd);
    }

    std::unique_ptr<GeometricPrimitive> Graphics::CreateGeometricPrimitive(
        std::unique_ptr<GeometricPrimitive> _primitive
    )
    {
        if (_primitive != nullptr)
        {
            ResourceUploadBatch resourceUpload(m_device.Get());
            resourceUpload.Begin();
            _primitive->LoadStaticBuffers(m_device.Get(), resourceUpload);
            auto finished = resourceUpload.End(m_cmdQueue.Get());
            finished.wait();
        }

        return _primitive;
    }

    std::unique_ptr<GeometricPrimitive> Graphics::CreateGeometricPrimitive(
        GeometricPrimitive::VertexCollection& _vertices,
        GeometricPrimitive::IndexCollection& _indices
    )
    {
        auto prim = GeometricPrimitive::CreateCustom(_vertices, _indices, m_device.Get());
        if (prim != nullptr)
        {
            ResourceUploadBatch resourceUpload(m_device.Get());
            resourceUpload.Begin();
            prim->LoadStaticBuffers(m_device.Get(), resourceUpload);
            auto finished = resourceUpload.End(m_cmdQueue.Get());
            finished.wait();
        }
        return prim;
    }

    std::unique_ptr<MoltenVertexBuffer> Graphics::CreateVertexBuffer(
        std::vector<VertexPositionNormalTangentColorTextureSkinning>& _vertices,
        std::vector<uint16_t>& _indices
    )
    {
        std::unique_ptr<MoltenVertexBuffer> vb = std::make_unique<MoltenVertexBuffer>();
        ResourceUploadBatch uploadBatch(m_device.Get());
        uploadBatch.Begin();
        if (FAILED(CreateStaticBuffer(
            m_device.Get(),
            uploadBatch,
            _vertices.data(),
            _vertices.size(),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            &vb->vb
        )))
        {
            return nullptr;
        }

        vb->vbv.BufferLocation = vb->vb->GetGPUVirtualAddress();
        vb->vbv.SizeInBytes = _vertices.size() * sizeof(VertexPositionNormalTangentColorTextureSkinning);
        vb->vbv.StrideInBytes = sizeof(VertexPositionNormalTangentColorTextureSkinning);

        if (FAILED(CreateStaticBuffer(
            m_device.Get(),
            uploadBatch,
            _indices.data(),
            _indices.size(),
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            &vb->ib
        )))
        {
            return nullptr;
        }
        auto end = uploadBatch.End(m_cmdQueue.Get());
        end.wait();

        vb->ibv.BufferLocation = vb->ib->GetGPUVirtualAddress();
        vb->ibv.Format = DXGI_FORMAT_R16_UINT;
        vb->ibv.SizeInBytes = _indices.size() * sizeof(uint16_t);

        vb->count = _indices.size();
        return vb;
    }

    void Graphics::RenderGeometricPrimitive(
        std::unique_ptr<GeometricPrimitive>& _primitive,
        Matrix& _world
    )
    {
        auto& cmd = m_directPipeline.GetCmd(m_frameIndex);
        m_effect->SetWorld(_world);
        m_effect->Apply(*cmd);
        
        _primitive->Draw(*cmd);
    }

    void Graphics::RenderVertexBuffer(
        std::unique_ptr<MoltenVertexBuffer>& vb,
        std::unique_ptr<MoltenTexture>& mt,
        std::vector<Matrix>& bones,
        Matrix& _world
    )
    {
        auto& cmd = m_directPipeline.GetCmd(m_frameIndex);
        m_skinnedEffect->SetTexture(
            m_resourceDescriptors->GetGpuHandle(mt->index),
            m_states->AnisotropicClamp()
        );
        std::vector<XMMATRIX> matrices;
        for (auto& it : bones)
        {
            matrices.push_back(it);
        }
        m_skinnedEffect->SetWorld(_world);
        m_skinnedEffect->SetBoneTransforms(matrices.data(), matrices.size());
        m_skinnedEffect->Apply(*cmd);
       
        cmd->IASetVertexBuffers(0, 1, &vb->vbv);
        cmd->IASetIndexBuffer(&vb->ibv);
        cmd->DrawIndexedInstanced(vb->count, 1, 0, 0, 0);
    }

    std::unique_ptr<MoltenTexture> Graphics::CreateTexture(std::string _name)
    {
        HRESULT hr = S_OK;
        std::unique_ptr<MoltenTexture> texture = std::make_unique<MoltenTexture>();
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string path = "Assets/textures/" + _name + ".png";

        ResourceUploadBatch resourceUpload(m_device.Get());
        resourceUpload.Begin();

        hr = CreateWICTextureFromFile(
            m_device.Get(),
            resourceUpload,
            converter.from_bytes(path).c_str(),
            &texture->tex
        );
        if (FAILED(hr))
        {
            Log::Error("Failed to Create Texture (" + _name + ")\n ErrorCode: " + std::to_string(hr));
            return nullptr;
        }

        auto finished = resourceUpload.End(m_cmdQueue.Get());
        finished.wait();

        texture->index = 0;
        m_device->CreateShaderResourceView(
            texture->tex.Get(),
            nullptr,
            m_resourceDescriptors->GetCpuHandle(texture->index)
        );
        //m_resourceDescriptors->Increment();
        return texture;
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

        m_effect->Apply(*m_directPipeline.GetCmd(m_frameIndex));
        ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap(), m_states->Heap() };
        m_directPipeline.GetCmd(m_frameIndex)->SetDescriptorHeaps(std::size(heaps), heaps);
        m_effect->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());
        m_skinnedEffect->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());
        ImGui_ImplDX12_NewFrame();
    }

    //struct ConstBuffer
    //{
    //    Matrix model;
    //    Matrix normalMat;
    //    Matrix cam;
    //    std::array<Matrix, MAX_BONES> bones;
    //};
    //void Graphics::RenderIndexed(
    //    MMesh& _mesh,
    //    MTexture& _texture,
    //    std::array<Matrix, MAX_BONES>& _bones
    //)
    //{
    //    auto& cmd = m_directPipeline.GetCmd(m_frameIndex);

    //    cmd->SetGraphicsRootSignature(m_directPipeline.m_pipeline.m_rootSig.Get());
    //    cmd->SetPipelineState(m_directPipeline.m_pipeline.m_pipeline.Get());

    //    ConstBuffer* buff = reinterpret_cast<ConstBuffer*>(m_cbData[m_frameIndex] + m_currentOffset);
    //    buff->model = Matrix::Identity;//Matrix::CreateRotationZ(3.14159265f / 2.0f);
    //    buff->normalMat = buff->model;
    //    buff->cam = m_camera.ViewProjection().Transpose();
    //    buff->bones = _bones;

    //    cmd->SetGraphicsRootConstantBufferView(0, m_constBuffers[m_frameIndex]->GetGPUVirtualAddress() + m_currentOffset);
    //    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_dHeap->GetCPUDescriptorHandleForHeapStart(), m_currentDescriptor, m_srvIncrementSize);
    //    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    //    D3D12_RESOURCE_DESC texDesc = _texture.texture->GetDesc();
    //    srvDesc.Format = texDesc.Format;
    //    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
    //    srvDesc.Texture2D.MostDetailedMip = 0;
    //    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    //    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //    m_device->CreateShaderResourceView(
    //        _texture.texture.Get(),
    //        nullptr,
    //        srvHandle
    //    );
    //    CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(m_dHeap->GetGPUDescriptorHandleForHeapStart(), m_currentDescriptor, m_srvIncrementSize);
    //    cmd->SetGraphicsRootDescriptorTable(1, srvGpuHandle);
    //    (++m_currentDescriptor) %= m_numDescriptors;
    //    m_currentOffset += sizeof(ConstBuffer);
    //    cmd->IASetIndexBuffer(&_mesh.indexBuffer.ibView);
    //    for (int i = 0; i < _mesh.vertexBuffers.size(); i++)
    //    {
    //        cmd->IASetVertexBuffers(i, 1, &_mesh.vertexBuffers[i].vbView);
    //    }

    //    cmd->DrawIndexedInstanced(_mesh.indexBuffer.count, 1, 0, 0, 0);
    //}

    void Graphics::RenderImGui()
    {
        m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_imGuiHeap.GetAddressOf());
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
    }

    void Graphics::EndFrame()
    {
        m_currentOffset = 0;
        m_directPipeline.Execute(m_cmdQueue);

        ImGui::Begin("View");
        if (ImGui::IsWindowFocused())
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            float testDelta = ImGui::GetIO().MouseWheel;
            
            if (dragDelta.x == 0 && dragDelta.y == 0)
            {
                m_currDragDelta = { dragDelta.x, dragDelta.y };
            }

            float x = dragDelta.x - m_currDragDelta.x;
            float y = dragDelta.y - m_currDragDelta.y;
            Vector3 pos = m_camera.GetPosition();
            //m_camera.Pitch(-y * 0.1f);
            m_camera.MoveUp(y * 0.2f);
            m_currDragDelta = { dragDelta.x, dragDelta.y };
            //m_camera.Yaw(-x * 0.1f);
            m_camera.MoveRight(x * 0.2f);
            Vector3 tpos = m_camera.GetPosition();
            tpos.Normalize();
            pos = tpos * pos.Length();
            m_camera.SetPosition(pos);
            if (testDelta >= 1 || testDelta <= -1) 
            { 
                m_camera.MoveForward(testDelta); 
            }
            
            m_camera.LookAt(m_camera.GetPosition(), { 0, 0, 0 }, { 0, 1, 0 });
        }
        D3D12_VIEWPORT vp = m_directPipeline.GetViewport();
        float aspect = vp.Width / vp.Height;
        float inverseAspect = vp.Height / vp.Width;
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
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_imGuiHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex + 1, m_srvIncrementSize);
        m_device->CreateShaderResourceView(m_directPipeline.GetBuffer(m_frameIndex).Get(), nullptr, srvHandle);
        ImGui::SetCursorPos(cursorPos);
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(m_imGuiHeap->GetGPUDescriptorHandleForHeapStart(), m_frameIndex + 1, m_srvIncrementSize);
        ImGui::Image(
            (ImTextureID)srvGpuHandle.ptr,
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

        m_swapChain.swapChain->Present(0, 0);
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
            m_resize = false;
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

    IGraphics& IGraphics::Get()
    {
        static Graphics graphics;
        return graphics;
    }
}
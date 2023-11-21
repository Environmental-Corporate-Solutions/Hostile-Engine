#include "stdafx.h"
#include "Graphics.h"
#include <locale>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <misc/cpp/imgui_stdlib.h>
#include <DirectXTex.h>

#include <directxtk12/WICTextureLoader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/BufferHelpers.h>

#include <directxtk12/DirectXHelpers.h>
#include "ImGuizmo.h"
#include "Resources/Material.h"
using namespace DirectX;

namespace Hostile
{
    std::wstring ConvertToWideString(std::string const& _str)
    {
        std::wstring wStr;
        int convertResult = MultiByteToWideChar(
            CP_UTF8, 0, _str.c_str(), static_cast<int>(_str.size()), nullptr, 0);
        if (convertResult > 0)
        {
            wStr.resize(convertResult);
            MultiByteToWideChar(CP_UTF8, 0, _str.c_str(), 
                static_cast<int>(_str.size()), wStr.data(), static_cast<int>(wStr.size()));
        }

        return wStr;
    }

    DXGI_FORMAT FormatFromString(const std::string& _str)
    {
        if (_str == "R8G8B8A8_UNORM")
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        else if (_str == "R32G32B32A32_FLOAT")
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        else if (_str == "D24_UNORM_S8_UINT")
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        else if (_str == "D32_FLOAT")
            return DXGI_FORMAT_D32_FLOAT;
        else if (_str == "R32_FLOAT")
            return DXGI_FORMAT_R32_FLOAT;
        return DXGI_FORMAT_UNKNOWN;
    }

    //-------------------------------------------------------------------------
    // GRAPHICS
    //-------------------------------------------------------------------------
    bool Graphics::Init(GLFWwindow* _pWindow)
    {
        m_device.Init();
        ResourceLoader::Init(m_device);
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        ThrowIfFailed(m_swap_chain.Init(m_device.Device(), m_device.Adapter(), 
            m_device.Queue(), _pWindow));

        
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        for (auto& it : m_cmds)
        {
            it.Init(m_device.Device());
        }

        for (auto& it : m_draw_cmds)
            it.Init(m_device.Device());

        m_frame_index = 0;
        ImGui_ImplDX12_Init(
            m_device.Device().Get(),
            g_frame_count,
            m_swap_chain.m_format,
            m_device.ResourceHeap().Heap(),
            m_device.ResourceHeap().GetFirstCpuHandle(),
            m_device.ResourceHeap().GetFirstGpuHandle()
        );
        m_device.ResourceHeap().Allocate();

        m_graphics_memory = std::make_unique<GraphicsMemory>(
            m_device.Device().Get());

        m_states = std::make_unique<CommonStates>(m_device.Device().Get());
        ResourceUploadBatch resourceUpload(m_device.Device().Get());
        resourceUpload.Begin();
        auto uploadResourcesFinished = resourceUpload.End(
            m_device.Queue().Get());
        uploadResourcesFinished.wait();
        RenderTargetState sceneState(
            m_swap_chain.m_format,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );

        RenderTarget::RenderTargetCreateInfo create_info(
            { 1920, 1080 },
            DXGI_FORMAT_R8G8B8A8_UNORM
        );
        m_gbuffer[static_cast<size_t>(GBuffer::Color)] = RenderTarget::Create(m_device, create_info);
        create_info.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        m_gbuffer[static_cast<size_t>(GBuffer::WorldPos)] = RenderTarget::Create(m_device, create_info);
        m_gbuffer[static_cast<size_t>(GBuffer::Normal)] = RenderTarget::Create(m_device, create_info);

        m_lighting_pipeline = Pipeline::Create(m_device, "Assets/Pipelines/Lighting.json");
        for (auto& index : m_light_index)
        {
            index = m_device.ResourceHeap().Allocate();
        }
        m_frame = ResourceLoader::Get().GetOrLoadResource<VertexBuffer>("Square");

        ComPtr<ID3D12Device5> device;
        HRESULT hr = m_device->QueryInterface(IID_PPV_ARGS(&device));
        if (SUCCEEDED(hr))
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
            m_device->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
            m_device->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

            if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
                Log::Critical("No Raytracing :(");
            else
                Log::Critical("RAYTRACING!!!");

            Log::Info("D3D12_RENDERPASS_TIER_" + std::to_string(options5.RenderPassesTier));
        }

        return false;
    }

    void Graphics::SetCamera(const Vector3& _position, const Matrix& _matrix)
    {
        m_camera_matrix = _matrix;
        m_camera_position = _position;
    }

    void Graphics::Draw(DrawCall& _draw_call)
    {
        _draw_call.instance.m_material->GetPipeline()->AddInstance(_draw_call);
    }

    void Graphics::AddLight(const Light& _light)
    {
        m_lights.push_back(_light);
    }

    std::shared_ptr<IRenderTarget> Graphics::CreateRenderTarget(UINT _i)
    {
        RenderTarget::RenderTargetCreateInfo create_info{};
        create_info.dimensions = Vector2{ 1920, 1080 };
        create_info.format = (_i == 0) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R32_FLOAT;

        auto rt = RenderTarget::Create(m_device, create_info);//std::make_shared<RenderTarget>(m_device.Device(), m_device.ResourceHeap(), DXGI_FORMAT_R8G8B8A8_UNORM, Vector2{1920, 1080});

        if (rt)
        {
            m_render_targets.push_back(rt);
        }

        return rt;
    }

    std::shared_ptr<DepthTarget> Graphics::CreateDepthTarget()
    {
        std::shared_ptr<DepthTarget> md = std::make_shared<DepthTarget>();

        md->heap = std::make_unique<DescriptorHeap>
    	(
            m_device.Device().Get(),   //device 
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            g_frame_count
        );

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R24G8_TYPELESS,
            1920, 1080, 1U, 0U, 1U, 0U,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
        for (int i = 0; i < g_frame_count; i++)
        {
            CD3DX12_CLEAR_VALUE clearValue(DXGI_FORMAT_D24_UNORM_S8_UINT, 1, 0x0);
            if (FAILED(m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValue,
                IID_PPV_ARGS(&md->textures[i])
            )))
            {
                return nullptr;
            }

            D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
            dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
            dsv_desc.Texture2D.MipSlice = 0;
            m_device->CreateDepthStencilView(
                md->textures[i].Get(),
                &dsv_desc,
                md->heap->GetCpuHandle(i)
            );
            md->dsvs[i] = md->heap->GetCpuHandle(i);

            UINT index = m_device.ResourceHeap().Allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
            srv_desc.Texture2D.MipLevels = 1;
            srv_desc.Texture2D.MostDetailedMip = 0;
            srv_desc.Texture2D.PlaneSlice = 1;
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            
            md->srvs[i] = m_device.ResourceHeap().GetGpuHandle(index);
            m_device->CreateShaderResourceView(
                md->textures[i].Get(),
                &srv_desc,
                m_device.ResourceHeap().GetCpuHandle(index)
            );
        }
        md->frameIndex = m_frame_index;
        m_depth_targets.push_back(md);

        return md;
    }

    IReadBackBufferPtr Graphics::CreateReadBackBuffer(IRenderTargetPtr& _render_target)
    {
        ReadBackBuffer::ReadBackBufferCreateInfo info{};
        info.dimensions = Vector2{ 1920, 1080 };
        info.format = DXGI_FORMAT_R32_FLOAT;
        IReadBackBufferPtr buffer = ReadBackBuffer::Create(m_device, info);
        return buffer;
    }

    void Graphics::RenderObjects()
    {
        auto& cmd = m_draw_cmds[m_frame_index];
        cmd.Reset(nullptr);
        std::array heaps = { m_device.ResourceHeap().Heap(), m_states->Heap() };
        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        GraphicsResource lights_resource = 
            m_graphics_memory->Allocate(m_lights.size() * sizeof(Light));
        memcpy(
            lights_resource.Memory(), 
            m_lights.data(), 
            sizeof(Light) * m_lights.size()
        );

        ShaderConstants shaderConstants{};
        shaderConstants.view_projection = m_camera_matrix;

        XMStoreFloat3A(&shaderConstants.camera_position, (XMVECTOR)m_camera_position);

        GraphicsResource scene_resource = m_graphics_memory->AllocateConstant<ShaderConstants>(shaderConstants);

        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, static_cast<size_t>(GBuffer::Count)> rtvs = {
            m_gbuffer[static_cast<size_t>(GBuffer::Color)]->GetRTV(),
            m_gbuffer[static_cast<size_t>(GBuffer::WorldPos)]->GetRTV(),
            m_gbuffer[static_cast<size_t>(GBuffer::Normal)]->GetRTV()
        };

        cmd->OMSetRenderTargets(
            rtvs.size(), 
            rtvs.data(), 
            false, 
            &m_depth_targets[0]->dsvs[m_depth_targets[0]->frameIndex]
        );

        cmd->RSSetViewports(1, &m_gbuffer[static_cast<size_t>(GBuffer::Color)]->GetViewport());
        cmd->RSSetScissorRects(1, &m_gbuffer[static_cast<size_t>(GBuffer::Color)]->GetScissor());

        for (auto& [name, pipeline] : ResourceLoader::Get().m_resource_cache[Pipeline::TypeID()])
        {
            std::dynamic_pointer_cast<Pipeline>(pipeline)->Draw(cmd, scene_resource, lights_resource);
        }

        for (auto& render_target : m_gbuffer)
        {
            render_target->Submit(cmd);
        }

        std::array rtvs2 = {
            m_render_targets[0]->GetRTV(),
            m_render_targets[1]->GetRTV()
        };
        cmd->OMSetRenderTargets(rtvs2.size(), rtvs2.data(), false, nullptr);

        cmd->SetPipelineState(m_lighting_pipeline->m_pipeline.Get());
        cmd->SetGraphicsRootSignature(m_lighting_pipeline->m_root_signature.Get());
        cmd->SetGraphicsRootDescriptorTable(2, m_gbuffer[static_cast<size_t>(GBuffer::Color)]->GetSRV());
        cmd->SetGraphicsRootDescriptorTable(3, m_gbuffer[static_cast<size_t>(GBuffer::WorldPos)]->GetSRV());
        cmd->SetGraphicsRootDescriptorTable(4, m_gbuffer[static_cast<size_t>(GBuffer::Normal)]->GetSRV());
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = lights_resource.ResourceOffset() / sizeof(Light);
        srv_desc.Buffer.NumElements = m_lights.size();
        srv_desc.Buffer.StructureByteStride = sizeof(Light);
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        
        m_device->CreateShaderResourceView(
            lights_resource.Resource(),
            &srv_desc,
            m_device.ResourceHeap().GetCpuHandle(m_light_index[m_frame_index])
        );
        m_lighting_pipeline->DrawInstanced(
            cmd,
            m_frame,
            scene_resource,
            m_device.ResourceHeap().GetGpuHandle(m_light_index[m_frame_index]),
            m_lights.size()
        );
        m_lights.clear();

        for (auto const& it : m_render_targets)
        {
            it->Submit(cmd);
            it->IncrementFrameIndex();
        }

        cmd->Close();
        std::array<ID3D12CommandList*, 1> lists = { *cmd };
        m_device.Queue()->ExecuteCommandLists(
            static_cast<UINT>(lists.size()), lists.data());
        ++cmd.m_fenceValue;
        m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
        m_device.Queue()->Wait(cmd.m_fence.Get(), cmd.m_fenceValue);
    }




    void Graphics::BeginFrame()
    {
        {
            CommandList& cmd = m_cmds[m_frame_index];
            cmd.Wait();

            cmd.Reset(nullptr);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swap_chain.GetBackBuffer(
                m_frame_index);
            cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
            std::array color = { 0.3411f, 0.2117f, 0.0196f, 1.0f };


            std::array barriers = {
                CD3DX12_RESOURCE_BARRIER::Transition(
                m_swap_chain.rtvs[m_swap_chain.swapChain->
                    GetCurrentBackBufferIndex()].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET)
            };
            cmd->ResourceBarrier(
                static_cast<UINT>(barriers.size()), barriers.data());

            cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmd->RSSetViewports(1, &m_swap_chain.m_viewport);
            cmd->RSSetScissorRects(1, &m_swap_chain.m_scissorRect);

            cmd->ClearRenderTargetView(rtv, color.data(), 0, nullptr);


            for (auto const& it : m_render_targets)
            {
                it->Clear(cmd);
            }

            for (auto& it : m_gbuffer)
            {
                it->Clear(cmd);
            }

            for (auto const& it : m_depth_targets)
            {
                it->frameIndex = m_frame_index;
                cmd->ClearDepthStencilView(
                    it->dsvs[it->frameIndex], 
                    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
                    1, 0x0, 0, nullptr);
            }
            cmd->Close();
            std::array<ID3D12CommandList*, 1> lists = { *cmd };
            m_device.Queue()->ExecuteCommandLists(
                static_cast<UINT>(lists.size()), lists.data());
            ++cmd.m_fenceValue;
            m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
            cmd.Wait();
            ImGui_ImplDX12_NewFrame();
        }
    }

    void Graphics::RenderImGui()
    {
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(
            ImGui::GetDrawData(), *m_cmds[m_frame_index]);
    }

    void Graphics::EndFrame()
    {
        CommandList& cmd = m_cmds[m_frame_index];
        cmd.Reset(nullptr);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = 
            m_swap_chain.GetBackBuffer(m_frame_index);

        std::array heaps = { 
            m_device.ResourceHeap().Heap(), m_states->Heap() };

        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
        this->RenderObjects();
        cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
        RenderImGui();

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_swap_chain.rtvs[m_swap_chain.swapChain->
            GetCurrentBackBufferIndex()].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        cmd->ResourceBarrier(1, &barrier);
        cmd->Close();
        std::array<ID3D12CommandList*, 1> lists = { *cmd };
        m_device.Queue()->ExecuteCommandLists(1, lists.data());

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, (void*)lists.data());
        }

        m_swap_chain.swapChain->Present(0, 0);
        m_graphics_memory->Commit(m_device.Queue().Get());
        ++cmd.m_fenceValue;
        m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
        m_frame_index++;
        m_frame_index %= g_frame_count;

        if (m_resize)
        {
            for (auto& it : m_cmds)
            {
                it.Wait();
            }
            m_swap_chain.Resize(m_resize_width, m_resize_height);
            m_resize = false;
        }
    }

    void Graphics::OnResize(UINT _width, UINT _height)
    {
        m_resize = true;
        m_resize_width = _width;
        m_resize_height = _height;
    }

    void Graphics::Shutdown()
    {
        ImGui_ImplDX12_Shutdown();

        for (auto& it : m_cmds)
        {
            it.Shutdown();
        }
        m_swap_chain.rtvHeap.Reset();

        for (auto& it : m_swap_chain.rtvs)
        {
            it.Reset();
        }
        m_swap_chain.swapChain.Reset();
    }

    IGraphics& IGraphics::Get()
    {
        static Graphics graphics;
        return graphics;
    }
}
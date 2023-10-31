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
using namespace DirectX;

namespace Hostile
{
    std::wstring ConvertToWideString(std::string const& _str)
    {
        std::wstring wStr;
        int convertResult = MultiByteToWideChar(CP_UTF8, 0, _str.c_str(), static_cast<int>(_str.size()), nullptr, 0);
        if (convertResult > 0)
        {
            wStr.resize(convertResult);
            MultiByteToWideChar(CP_UTF8, 0, _str.c_str(), static_cast<int>(_str.size()), wStr.data(), static_cast<int>(wStr.size()));
        }

        return wStr;
    }

    //-------------------------------------------------------------------------
    // GRAPHICS
    //-------------------------------------------------------------------------
    bool Graphics::Init(GLFWwindow* _pWindow)
    {
        m_device.Init();
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        ThrowIfFailed(m_swapChain.Init(m_device.Device(), m_device.Adapter(), m_device.Queue(), _pWindow));

        for (auto& it : m_cmds)
        {
            it.Init(m_device.Device());
        }

        for (auto& it : m_draw_cmds)
            it.Init(m_device.Device());

        m_frameIndex = 0;
        ImGui_ImplDX12_Init(
            m_device.Device().Get(),
            g_frame_count,
            m_swapChain.m_format,
            m_device.ResourceHeap().Heap(),
            m_device.ResourceHeap().GetFirstCpuHandle(),
            m_device.ResourceHeap().GetFirstGpuHandle()
        );
        m_device.ResourceHeap().Allocate();

        m_graphicsMemory = std::make_unique<GraphicsMemory>(m_device.Device().Get());

        m_states = std::make_unique<CommonStates>(m_device.Device().Get());
        ResourceUploadBatch resourceUpload(m_device.Device().Get());
        resourceUpload.Begin();
        auto uploadResourcesFinished = resourceUpload.End(m_device.Queue().Get());
        uploadResourcesFinished.wait();
        RenderTargetState sceneState(
            m_swapChain.m_format,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );

        ComPtr<ID3D12Device5> device;
        HRESULT hr = m_device->QueryInterface(IID_PPV_ARGS(&device));
        if (SUCCEEDED(hr))
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

            if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
                Log::Critical("No Raytracing :(");
            else
                Log::Critical("RAYTRACING!!!");

            Log::Info("D3D12_RENDERPASS_TIER_" + std::to_string(options5.RenderPassesTier));
        }

        {
            Texture texture;
            m_device.LoadTexture("sky-5.png", texture);

            GetOrLoadMesh("Cube");
            GetOrLoadMesh("Sphere");
        }

        return false;
    }

    PipelinePtr Graphics::GetOrLoadPipeline(std::string const& _name)
    {
        if (m_pipelines.find(_name) != m_pipelines.end())
        {
            return m_pipelines[_name];
        }
        else
        {
            m_pipelines[_name] = Pipeline::Create(m_device, _name);
            
            return m_pipelines[_name];
        }

    }

    VertexBufferPtr Graphics::GetOrLoadMesh(std::string const& _name)
    {
        if (m_meshes.find(_name) != m_meshes.end())
            return m_meshes[_name];

        VertexCollection vertices;
        IndexCollection indices;
        if (_name == "Cube")
        {
            ComputeBox(vertices, indices, XMFLOAT3(1, 1, 1), true, false);
        }
        else if (_name == "Sphere")
        {
            ComputeSphere(vertices, indices, 1.0f, 16, true, false);
        }
        else if (_name == "Dodecahedron")
        {
            ComputeDodecahedron(vertices, indices, 1, true);
        }
        else if (_name == "Cylinder")
        {
            ComputeCylinder(vertices, indices, 1, 1, 8, true);
        }
        else if (_name == "Square")
        {
            vertices.push_back({ Vector3{-1, -1, 0}, Vector3{0, 0, 0}, Vector2{0, 1} });
            vertices.push_back({ Vector3{-1,  1, 0}, Vector3{0, 0, 0}, Vector2{0, 0} });
            vertices.push_back({ Vector3{ 1,  1, 0}, Vector3{0, 0, 0}, Vector2{1, 0} });
            vertices.push_back({ Vector3{ 1, -1, 0}, Vector3{0, 0, 0}, Vector2{0, 1} });
            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);
            indices.push_back(2);
            indices.push_back(3);
        }

        try
        {
            VertexBufferPtr vb = std::make_shared<VertexBuffer>(CreateVertexBuffer(vertices, indices));
            vb->name = _name;
            m_meshes[_name] = vb;
        }
        catch (DirectXException& e)
        {
            Log::Error(e.what() + e.error());
        }


        return m_meshes[_name];
    }

    MaterialPtr Graphics::GetOrLoadMaterial(const std::string& _name)
    {
        if (m_materials.find(_name) != m_materials.end())
            return m_materials[_name];

        m_materials[_name] = std::make_shared<Material>(); 
        m_materials[_name]->name = _name;

        return m_materials[_name];
    }

    TexturePtr Graphics::GetOrLoadTexture(const std::string& _name)
    {
        if (m_textures.find(_name) == m_textures.end())
        {
            Texture t;
            m_device.LoadTexture(_name, t);
            m_textures[_name] = std::make_shared<Texture>(t);
        }
        return m_textures[_name];
    }

    void Graphics::SetLight(UINT _light, bool _active)
    {
        m_lights[_light].lightColor.w = (float)_active;
    }

    void Graphics::SetLight(UINT _light, const Vector3& _position, const Vector3& _color)
    {
        m_lights[_light].lightColor.x = _color.x;
        m_lights[_light].lightColor.y = _color.y;
        m_lights[_light].lightColor.z = _color.z;

        XMStoreFloat3A(&m_lights[_light].lightPosition, _position);
    }

    void Graphics::Draw(DrawCall& _draw_call)
    {
        _draw_call.instance.m_material->m_pipeline->AddInstance(_draw_call);
    }

    VertexBuffer Graphics::CreateVertexBuffer(
        VertexCollection& _vertices,
        IndexCollection& _indices
    )
    {
        using namespace DirectX;
        VertexBuffer vb;
        ResourceUploadBatch uploadBatch(m_device.Device().Get());
        uploadBatch.Begin();
        ThrowIfFailed(CreateStaticBuffer(
            m_device.Device().Get(),
            uploadBatch,
            _vertices.data(),
            _vertices.size(),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            &vb.vb
        ));

        vb.vbv.BufferLocation = vb.vb->GetGPUVirtualAddress();
        vb.vbv.SizeInBytes = static_cast<UINT>(_vertices.size() * sizeof(PrimitiveVertex));
        vb.vbv.StrideInBytes = sizeof(PrimitiveVertex);

        ThrowIfFailed(CreateStaticBuffer(
            m_device.Device().Get(),
            uploadBatch,
            _indices.data(),
            _indices.size(),
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            &vb.ib
        ));
        auto end = uploadBatch.End(m_device.Queue().Get());
        end.wait();

        vb.ibv.BufferLocation = vb.ib->GetGPUVirtualAddress();
        vb.ibv.Format = DXGI_FORMAT_R16_UINT;
        vb.ibv.SizeInBytes = static_cast<UINT>(_indices.size() * sizeof(uint16_t));

        vb.count = static_cast<UINT>(_indices.size());
        return vb;
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

        md->heap = std::make_unique<DescriptorHeap>(m_device.Device().Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, g_frame_count);

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
        md->frameIndex = m_frameIndex;
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
        auto& cmd = m_draw_cmds[m_frameIndex];
        cmd.Reset(nullptr);
        std::array heaps = { m_device.ResourceHeap().Heap(), m_states->Heap() };
        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        GraphicsResource lights_resource = m_graphicsMemory->Allocate(m_lights.size() * sizeof(Light));
        memcpy(lights_resource.Memory(), m_lights.data(), sizeof(Light) * m_lights.size());
        

        auto const& renderTarget = m_render_targets[0];
        ShaderConstants shaderConstants{};
        shaderConstants.viewProjection = renderTarget->GetView() * renderTarget->GetProjection();

        XMStoreFloat3A(&shaderConstants.cameraPosition, (XMVECTOR)renderTarget->GetCameraPosition());

        GraphicsResource scene_resource = m_graphicsMemory->AllocateConstant<ShaderConstants>(shaderConstants);

        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvs = { renderTarget->GetRTV(), m_render_targets[1]->GetRTV() };
        cmd->OMSetRenderTargets(rtvs.size(), rtvs.data(), false, &m_depth_targets[0]->dsvs[m_depth_targets[0]->frameIndex]);
        cmd->RSSetViewports(1, &renderTarget->GetViewport());
        cmd->RSSetScissorRects(1, &renderTarget->GetScissor());

        for (auto& [name, pipeline] : m_pipelines)
        {
            pipeline->Draw(cmd, scene_resource, lights_resource);
        }

        for (auto const& it : m_render_targets)
        {
            it->Submit(cmd);
            it->IncrementFrameIndex();
        }

        cmd->Close();
        std::array<ID3D12CommandList*, 1> lists = { *cmd };
        m_device.Queue()->ExecuteCommandLists(static_cast<UINT>(lists.size()), lists.data());
        ++cmd.m_fenceValue;
        m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
        m_device.Queue()->Wait(cmd.m_fence.Get(), cmd.m_fenceValue);
    }




    void Graphics::BeginFrame()
    {
        {
            CommandList& cmd = m_cmds[m_frameIndex];
            cmd.Wait();

            cmd.Reset(nullptr);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);
            cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
            std::array color = { 0.3411f, 0.2117f, 0.0196f, 1.0f };


            std::array barriers = {
                CD3DX12_RESOURCE_BARRIER::Transition(
                m_swapChain.rtvs[m_swapChain.swapChain->GetCurrentBackBufferIndex()].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            )
            };
            cmd->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmd->RSSetViewports(1, &m_swapChain.m_viewport);
            cmd->RSSetScissorRects(1, &m_swapChain.m_scissorRect);

            cmd->ClearRenderTargetView(rtv, color.data(), 0, nullptr);


            std::vector<D3D12_RESOURCE_BARRIER> bars;
            for (auto const& it : m_render_targets)
            {
                bars.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                    it->GetTexture().Get(),
                    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET
                ));
            }
            cmd->ResourceBarrier(static_cast<UINT>(bars.size()), bars.data());

            for (auto const& it : m_render_targets)
            {
                it->Clear(cmd);
            }
            for (auto const& it : m_depth_targets)
            {
                it->frameIndex = m_frameIndex;
                cmd->ClearDepthStencilView(it->dsvs[it->frameIndex], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0x0, 0, nullptr);
            }
            cmd->Close();
            std::array<ID3D12CommandList*, 1> lists = { *cmd };
            m_device.Queue()->ExecuteCommandLists(static_cast<UINT>(lists.size()), lists.data());
            ++cmd.m_fenceValue;
            m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
            cmd.Wait();
            ImGui_ImplDX12_NewFrame();
        }
    }

    void Graphics::RenderImGui()
    {
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
    }

    void Graphics::EndFrame()
    {
        CommandList& cmd = m_cmds[m_frameIndex];
        cmd.Reset(nullptr);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);

        std::array heaps = { m_device.ResourceHeap().Heap(), m_states->Heap() };

        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
        this->RenderObjects();
        cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
        RenderImGui();

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_swapChain.rtvs[m_swapChain.swapChain->GetCurrentBackBufferIndex()].Get(),
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

        m_swapChain.swapChain->Present(0, 0);
        m_graphicsMemory->Commit(m_device.Queue().Get());
        ++cmd.m_fenceValue;
        m_device.Queue()->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
        m_frameIndex++;
        m_frameIndex %= g_frame_count;

        if (m_resize)
        {
            for (auto& it : m_cmds)
            {
                it.Wait();
            }
            m_swapChain.Resize(m_resizeWidth, m_resizeHeight);
            m_resize = false;
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
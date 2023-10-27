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
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
        m_cmdQueue->SetName(L"Command Queue");
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        ThrowIfFailed(m_swapChain.Init(m_device.Device(), m_device.Adapter(), m_cmdQueue, _pWindow));

        for (auto& it : m_cmds)
        {
            it.Init(m_device.Device());
        }
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
        auto uploadResourcesFinished = resourceUpload.End(m_cmdQueue.Get());
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

            LoadMesh("Cube");
            LoadMesh("Sphere");
        }

        return false;
    }

    void Graphics::LoadPipeline(std::string const& _name)
    {
        m_pipelines[_name] = Pipeline::Create(m_device, _name);
    }

    MeshID Graphics::LoadMesh(std::string const& _name)
    {
        if (m_meshIDs.find(_name) != m_meshIDs.end())
            return m_meshIDs[_name];

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

        MeshID mesh{ INVALID_ID };
        try
        {
            VertexBuffer vb = CreateVertexBuffer(vertices, indices);
            mesh = m_currentMeshID;
            m_currentMeshID++;
            m_meshIDs[_name] = mesh;
            m_meshes[mesh] = vb;
        }
        catch (DirectXException& e)
        {
            Log::Error(e.what() + e.error());
        }


        return mesh;
    }

    MaterialID Graphics::LoadMaterial(std::string const& _name, std::string const& _pipeline)
    {
        if (m_materialIDs.find(_name) != m_materialIDs.end())
            return m_materialIDs[_name];
        if (m_pipelines.find(_pipeline) == m_pipelines.end())
            return MaterialID{ INVALID_ID };

        MaterialID material = m_currentMaterial;
        m_currentMaterial++;
        m_materialIDs[_name] = material;
        m_materials[material].m_materialInputs = m_pipelines[_pipeline].MaterialInputs();
        m_materials[material].name = _name;
        m_materials[material].size = m_pipelines[_pipeline].MaterialInputsSize();
        m_materials[material].pipeline = _pipeline;

        return material;
    }

    MaterialID Graphics::CreateMaterial(std::string const& _name)
    {
        if (m_materialIDs.find(_name) != m_materialIDs.end())
            return MaterialID{ INVALID_ID };

        MaterialID material = m_currentMaterial;
        m_currentMaterial++;
        m_materialIDs[_name] = material;
        m_materials[material] = Pipeline::Material();

        return material;
    }

    MaterialID Graphics::CreateMaterial(std::string const& _name, MaterialID const& _id)
    {
        return -1;
    }

    InstanceID Graphics::CreateInstance(MeshID const& _mesh, MaterialID const& _material, UINT32 _id)
    {
        ObjectInstance instance{};
        instance.material = _material;
        instance.world = Matrix::Identity;
        instance.mesh = _mesh;
        instance.id = _id;

        m_objectInstances.push_back(instance);
        InstanceID id = m_objectInstances.size() - 1;
        m_meshInstances[_mesh].push_back(id);

        return id;
    }


    LightID Graphics::CreateLight()
    {
        for (size_t i = 0; i < m_lights.size(); i++)
        {
            if (m_lights[i].lightColor.w != 1)
            {
                m_lights[i].lightColor.w = 1;
                return i;
            }
        }

        return false;
    }

    bool Graphics::DestroyLight(LightID const& _light)
    {
        if (_light == INVALID_ID)
            return false;

        m_lights[(uint64_t)_light].lightColor.w = 0;

        return true;
    }

    bool Graphics::UpdateLight(LightID const& _light, Vector3 const& _position, Vector3 const& _color)
    {
        if (_light == INVALID_ID)
            return false;

        m_lights[(uint64_t)_light].lightPosition = { _position.x, _position.y, _position.z };
        m_lights[(uint64_t)_light].lightColor.x = _color.x;
        m_lights[(uint64_t)_light].lightColor.y = _color.y;
        m_lights[(uint64_t)_light].lightColor.z = _color.z;

        return true;
    }

    bool Graphics::UpdateInstance(InstanceID const& _instance, Matrix const& _world)
    {
        m_objectInstances[(uint64_t)_instance].world = _world;
        return true;
    }

    bool Graphics::UpdateInstance(InstanceID const& _instance, MeshID const& _id)
    {
        MeshID previousID = m_objectInstances[(uint64_t)_instance].mesh;
        if (previousID != INVALID_ID)
        {
            auto& instanceList = m_meshInstances[previousID];
            auto& it = std::find(instanceList.begin(), instanceList.end(), _instance);
            if (it != instanceList.end())
            {
                instanceList.erase(it);
            }
        }
        m_objectInstances[(uint64_t)_instance].mesh = _id;
        m_meshInstances[_id].push_back(_instance);
        return true;
    }

    bool Graphics::UpdateInstance(InstanceID const& _instance, MaterialID const& _id)
    {
        if (_instance != INVALID_ID)
        {
            m_objectInstances[(uint64_t)_instance].material = _id;
        }
        return false;
    }

    //bool Graphics::UpdateMaterial(MaterialID const& _id, PBRMaterial const& _material)
    //{
    //    if (m_materials.find(_id) != m_materials.end())
    //    {
    //        m_materials[_id] = _material;
    //        return true;
    //    }
    //    return false;
    //}

    void Graphics::ImGuiMaterialPopup(MaterialID const& _id)
    {

        if (ImGui::BeginPopup("Material Editor"))
        {
            for (auto& input : m_materials[_id].m_materialInputs)
            {
                switch (input.type)
                {
                case Pipeline::MaterialInput::Type::FLOAT:
                    ImGui::SliderFloat(input.name.c_str(), &std::get<float>(input.value), 0.0f, 1.0f);
                    break;
                case Pipeline::MaterialInput::Type::FLOAT2:
                    ImGui::SliderFloat2(input.name.c_str(), &std::get<Vector2>(input.value).x, 0.0f, 1.0f);
                    break;
                case Pipeline::MaterialInput::Type::FLOAT3:
                    ImGui::ColorEdit3(input.name.c_str(), &std::get<Vector3>(input.value).x);
                    break;
                case Pipeline::MaterialInput::Type::FLOAT4:
                    ImGui::ColorEdit4(input.name.c_str(), &std::get<Vector4>(input.value).x);
                    break;
                case Pipeline::MaterialInput::Type::TEXTURE:
                    ImGui::InputText(input.name.c_str(), &std::get<Texture>(input.value).name);
                    if (ImGui::Button("Apply"))
                    {
                        Texture& t = std::get<Texture>(input.value);
                        m_device.LoadTexture(t.name, t);
                    }
                    break;
                }
            }
            ImGui::EndPopup();
        }
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
        auto end = uploadBatch.End(m_cmdQueue.Get());
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
            m_renderTargets.push_back(rt);
        }

        return rt;
    }

    std::shared_ptr<DepthTarget> Graphics::CreateDepthTarget()
    {
        std::shared_ptr<DepthTarget> md = std::make_shared<DepthTarget>();

        md->heap = std::make_unique<DescriptorHeap>(m_device.Device().Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, g_frame_count);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D24_UNORM_S8_UINT,
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

            m_device->CreateDepthStencilView(
                md->textures[i].Get(),
                nullptr,
                md->heap->GetCpuHandle(i)
            );
            md->dsvs[i] = md->heap->GetCpuHandle(i);
        }
        md->frameIndex = m_frameIndex;
        m_depthTargets.push_back(md);

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
        auto& cmd = m_cmds[m_frameIndex];
        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        GraphicsResource lightsResource = m_graphicsMemory->Allocate(m_lights.size() * sizeof(Light));
        memcpy(lightsResource.Memory(), m_lights.data(), sizeof(Light) * m_lights.size());

        auto const& renderTarget = m_renderTargets[0];
        {
            std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvs = { renderTarget->GetRTV(), m_renderTargets[1]->GetRTV() };
            cmd->OMSetRenderTargets(rtvs.size(), rtvs.data(), false, &m_depthTargets[0]->dsvs[m_depthTargets[0]->frameIndex]);
            cmd->RSSetViewports(1, &renderTarget->GetViewport());
            cmd->RSSetScissorRects(1, &renderTarget->GetScissor());

            ShaderConstants shaderConstants{};
            shaderConstants.viewProjection = renderTarget->GetView() * renderTarget->GetProjection();

            XMStoreFloat3A(&shaderConstants.cameraPosition, (XMVECTOR)renderTarget->GetCameraPosition());

            GraphicsResource shaderConstantsResource = m_graphicsMemory->AllocateConstant<ShaderConstants>(shaderConstants);

            std::string currentPipeline = "";
            for (auto const& [meshInstance, instanceList] : m_meshInstances)
            {
                VertexBuffer const& vb = m_meshes[meshInstance];
                cmd->IASetVertexBuffers(0, 1, &vb.vbv);
                cmd->IASetIndexBuffer(&vb.ibv);

                for (auto const& instanceId : instanceList)
                {
                    auto const& instance = m_objectInstances[(uint64_t)instanceId];
                    GraphicsResource shaderObjectResource = m_graphicsMemory->AllocateConstant<ShaderObject>();
                    ShaderObject* shaderObject = (ShaderObject*)shaderObjectResource.Memory();
                    shaderObject->world = instance.world;
                    shaderObject->normalWorld = instance.world.Transpose().Invert();

                    Pipeline::Material& material = m_materials[instance.material];

                    Pipeline& p = m_pipelines[material.pipeline];
                    if (currentPipeline != p.Name())
                    {
                        p.Bind(cmd);
                        currentPipeline = p.Name();
                    }

                    int i = 0;
                    for (auto const& buffer : p.Buffers())
                    {
                        switch (buffer)
                        {
                        case Pipeline::Buffer::SCENE:
                            cmd->SetGraphicsRootConstantBufferView(i, shaderConstantsResource.GpuAddress());
                            break;
                        case Pipeline::Buffer::MATERIAL:
                        {
                            cmd->SetGraphicsRootConstantBufferView(1, lightsResource.GpuAddress());
                            if (i == 1)
                                i++;
                            GraphicsResource materialResource = m_graphicsMemory->Allocate(material.size);
                            UINT8* d = (UINT8*)materialResource.Memory();
                            for (auto& it : material.m_materialInputs)
                            {
                                switch (it.type)
                                {
                                case Pipeline::MaterialInput::Type::FLOAT:
                                    *reinterpret_cast<float*>(d) = std::get<float>(it.value);
                                    d += sizeof(float);
                                    break;
                                case Pipeline::MaterialInput::Type::FLOAT2:
                                    *reinterpret_cast<Vector2*>(d) = std::get<Vector2>(it.value);
                                    d += sizeof(Vector2);
                                    break;
                                case Pipeline::MaterialInput::Type::FLOAT3:
                                    *reinterpret_cast<Vector3*>(d) = std::get<Vector3>(it.value);
                                    d += sizeof(Vector3);
                                    break;
                                case Pipeline::MaterialInput::Type::FLOAT4:
                                    *reinterpret_cast<Vector4*>(d) = std::get<Vector4>(it.value);
                                    d += sizeof(Vector4);
                                    break;
                                }
                            }
                            cmd->SetGraphicsRootConstantBufferView(i, materialResource.GpuAddress());
                        }
                        break;
                        case Pipeline::Buffer::OBJECT:

                            cmd->SetGraphicsRootConstantBufferView(i, shaderObjectResource.GpuAddress());
                            break;
                        }
                        i++;
                    }

                    for (auto const& input : material.m_materialInputs)
                    {
                        switch (input.type)
                        {
                        case Pipeline::MaterialInput::Type::TEXTURE:
                            cmd->SetGraphicsRootDescriptorTable(i, m_device.ResourceHeap().GetGpuHandle(std::get<Texture>(input.value).index));
                            break;
                        }
                        i++;
                    }
                    cmd->DrawIndexedInstanced(vb.count, 1, 0, 0, 0);
                }
            }
        }
    }




    void Graphics::BeginFrame()
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
        for (auto const& it : m_renderTargets)
        {
            bars.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                it->GetTexture().Get(),
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            ));
        }
        cmd->ResourceBarrier(static_cast<UINT>(bars.size()), bars.data());

        for (auto const& it : m_renderTargets)
        {
            it->Clear(cmd);
        }
        for (auto const& it : m_depthTargets)
        {
            it->frameIndex = m_frameIndex;
            cmd->ClearDepthStencilView(it->dsvs[it->frameIndex], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0x0, 0, nullptr);
        }
        cmd->Close();
        std::array<ID3D12CommandList*, 1> lists = { *cmd };
        m_cmdQueue->ExecuteCommandLists(static_cast<UINT>(lists.size()), lists.data());
        ++cmd.m_fenceValue;
        m_cmdQueue->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
        cmd.Wait();
        ImGui_ImplDX12_NewFrame();

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

        std::vector<D3D12_RESOURCE_BARRIER> bars;
        for (auto const& it : m_renderTargets)
        {
            //bars.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            //    it->GetTexture().Get(),
            //    D3D12_RESOURCE_STATE_RENDER_TARGET,
            //    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
            //));
            it->Submit(cmd);
            it->IncrementFrameIndex();
        }
        //cmd->ResourceBarrier(static_cast<UINT>(bars.size()), bars.data());
        cmd->Close();
        std::array<ID3D12CommandList*, 1> lists = { *cmd };
        m_cmdQueue->ExecuteCommandLists(1, lists.data());

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, (void*)lists.data());
        }

        m_swapChain.swapChain->Present(0, 0);
        m_graphicsMemory->Commit(m_cmdQueue.Get());
        ++cmd.m_fenceValue;
        m_cmdQueue->Signal(cmd.m_fence.Get(), cmd.m_fenceValue);
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
        m_cmdQueue.Reset();
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
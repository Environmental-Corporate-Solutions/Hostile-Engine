#include "stdafx.h"
#include "Graphics.h"
#include <locale>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <DirectXTex.h>

#include <directxtk12/WICTextureLoader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/BufferHelpers.h>

#include <directxtk12/DirectXHelpers.h>

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
    // RenderTarget
    //-------------------------------------------------------------------------
    RenderTarget::RenderTarget(ComPtr<ID3D12Device> const& _device, DescriptorPile& _descriptorPile,
        DXGI_FORMAT _format, Vector2 _dimensions)
        : m_vp({ 0, 0, _dimensions.x, _dimensions.y, 0, 1 }), m_scissor(D3D12_RECT{ 0, 0, (long)_dimensions.x, (long)_dimensions.y }),
        m_clearValue({ _format, { 0, 0, 0, 1 } })
    {
        m_heap = std::make_unique<DescriptorHeap>(
            _device.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            FRAME_COUNT
        );

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            _format,
            (UINT64)_dimensions.x,
            (UINT)_dimensions.y
        );
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        for (UINT64 i = 0; i < FRAME_COUNT; i++)
        {
            ThrowIfFailed(_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &m_clearValue,
                IID_PPV_ARGS(&m_texture[i])
            ));

            m_srvIndices[i] = _descriptorPile.Allocate();
            m_srv[i] = _descriptorPile.GetGpuHandle(m_srvIndices[i]);
            m_rtv[i] = m_heap->GetCpuHandle(i);
            _device->CreateRenderTargetView(m_texture[i].Get(), nullptr, m_heap->GetCpuHandle(i));
            _device->CreateShaderResourceView(m_texture[i].Get(), nullptr, _descriptorPile.GetCpuHandle(m_srvIndices[i]));
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRTV() const
    {
        return m_rtv[m_frameIndex];
    }

    D3D12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetSRV() const
    {
        return m_srv[m_frameIndex];
    }

    ComPtr<ID3D12Resource>& RenderTarget::GetTexture()
    {
        return m_texture[m_frameIndex];
    }

    void RenderTarget::IncrementFrameIndex()
    {
        m_frameIndex++;
        m_frameIndex %= FRAME_COUNT;
    }

    void RenderTarget::Clear(CommandList& _cmd)
    {
        _cmd->ClearRenderTargetView(m_rtv[m_frameIndex], m_clearValue.Color, 0, nullptr);
    }

    D3D12_VIEWPORT RenderTarget::GetViewport() const
    {
        return m_vp;
    }

    D3D12_RECT RenderTarget::GetScissor() const
    {
        return m_scissor;
    }

    Vector2 RenderTarget::GetDimensions()
    {
        return { m_vp.Width, m_vp.Height };
    }

    UINT64 RenderTarget::GetPtr()
    {
        return m_srv[(m_frameIndex + 1) % FRAME_COUNT].ptr;
    }

    void RenderTarget::SetView(Matrix const& _view)
    {
        m_view = _view;
    }
    void RenderTarget::SetCameraPosition(Vector3 const& _cameraPosition)
    {
        m_cameraPosition = _cameraPosition;
    }
    void RenderTarget::SetProjection(Matrix const& _projection)
    {
        m_projection = _projection;
    }

    Matrix RenderTarget::GetView() const
    {
        return m_view;
    }

    Matrix RenderTarget::GetProjection() const
    {
        return m_projection;
    }

    Vector3 RenderTarget::GetCameraPosition() const
    {
        return m_cameraPosition;
    }

    //-------------------------------------------------------------------------
    // GRAPHICS
    //-------------------------------------------------------------------------
    bool Graphics::Init(GLFWwindow* _pWindow)
    {
        ComPtr<ID3D12Debug1> debug;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        debug->EnableDebugLayer();
        debug->SetEnableGPUBasedValidation(true);

        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings)));
        pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

        ComPtr<IDXGIAdapter> adapter;
        ThrowIfFailed(FindAdapter(adapter));

        ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
        m_cmdQueue->SetName(L"Command Queue");
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        ThrowIfFailed(m_swapChain.Init(m_device, adapter, m_cmdQueue, _pWindow));

        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_deviceFence)));
        m_deviceRemovedEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        assert(m_deviceRemovedEvent != nullptr);
        m_deviceFence->SetEventOnCompletion(UINT64_MAX, m_deviceRemovedEvent);

        RegisterWaitForSingleObject(
            &m_waitHandle,
            m_deviceRemovedEvent,
            OnDeviceRemoved,
            this,
            INFINITE,
            0
        );

        for (auto& it : m_cmds)
        {
            it.Init(m_device);
        }

        m_loadCmd.Init(m_device);

        m_resourceDescriptors = std::make_unique<DescriptorPile>(
            m_device.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            1024
        );
        m_frameIndex = 0;
        ImGui_ImplDX12_Init(
            m_device.Get(),
            FRAME_COUNT,
            m_swapChain.m_format,
            m_resourceDescriptors->Heap(),
            m_resourceDescriptors->GetFirstCpuHandle(),
            m_resourceDescriptors->GetFirstGpuHandle()
        );
        m_resourceDescriptors->Allocate();

        m_graphicsMemory = std::make_unique<GraphicsMemory>(m_device.Get());

        m_states = std::make_unique<CommonStates>(m_device.Get());
        ResourceUploadBatch resourceUpload(m_device.Get());
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
            m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

            if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
                Log::Critical("No Raytracing :(");
            else
                Log::Critical("RAYTRACING!!!");
        }

        {
            EffectPipelineStateDescription piped(
                &PrimitiveVertex::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthDefault,
                CommonStates::CullCounterClockwise,
                sceneState
            );
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> error;
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;
            hr = D3DCompileFromFile(L"Assets/Shaders/VertexShader.hlsl",
                nullptr, nullptr, "main", "vs_5_1",
                compileFlags, 0, &vertexShader, &error);
            if (FAILED(hr))
            {
                std::cout << "Error Compiling Vertex Shader: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
                throw DirectXException(hr);
            }

            ComPtr<ID3DBlob> pixelShader;
            hr = D3DCompileFromFile(L"Assets/Shaders/VertexShader.hlsl", nullptr, nullptr,
                "PSmain", "ps_5_1", compileFlags, 0, &pixelShader, &error);
            if (FAILED(hr))
            {
                std::cout << "Error Compiling Pixel Shader: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
                throw DirectXException(hr);
            }

            ThrowIfFailed(m_device->CreateRootSignature(0, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(&m_objectRootSignature)));

            D3D12_SHADER_BYTECODE vertexShaderByteCode{ vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
            D3D12_SHADER_BYTECODE pixelShaderByteCode{ pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

            piped.CreatePipelineState(
                m_device.Get(),
                m_objectRootSignature.Get(),
                vertexShaderByteCode,
                pixelShaderByteCode,
                &m_objectPipeline
            );
        }

        {
            EffectPipelineStateDescription piped(
                &PrimitiveVertex::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthNone,
                CommonStates::CullNone,
                sceneState
            );

            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> error;
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;
            hr = D3DCompileFromFile(L"Assets/Shaders/VertexShader.hlsl",
                nullptr, nullptr, "VSSkyboxMain", "vs_5_1",
                compileFlags, 0, &vertexShader, &error);
            if (FAILED(hr))
            {
                std::cout << "Error Compiling Vertex Shader: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
                throw DirectXException(hr);
            }

            ComPtr<ID3DBlob> pixelShader;
            hr = D3DCompileFromFile(L"Assets/Shaders/VertexShader.hlsl", nullptr, nullptr,
                "PSSkyboxMain", "ps_5_1", compileFlags, 0, &pixelShader, &error);
            if (FAILED(hr))
            {
                std::cout << "Error Compiling Pixel Shader: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
                throw DirectXException(hr);
            }

            ThrowIfFailed(m_device->CreateRootSignature(0, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(&m_skyboxRootSignature)));

            D3D12_SHADER_BYTECODE vertexShaderByteCode{ vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
            D3D12_SHADER_BYTECODE pixelShaderByteCode{ pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

            piped.CreatePipelineState(
                m_device.Get(),
                m_skyboxRootSignature.Get(),
                vertexShaderByteCode,
                pixelShaderByteCode,
                &m_skyboxPipeline
            );

            ResourceUploadBatch uploadBatch(m_device.Get());
            uploadBatch.Begin();
            ThrowIfFailed(CreateWICTextureFromFile(m_device.Get(), uploadBatch, L"Assets/textures/sky-5.png", &m_skyboxTexture));
            auto& f = uploadBatch.End(m_cmdQueue.Get());
            f.wait();
            m_skyboxTextureIndex = m_resourceDescriptors->Allocate();

            m_device->CreateShaderResourceView(
                m_skyboxTexture.Get(),
                nullptr,
                m_resourceDescriptors->GetCpuHandle(m_skyboxTextureIndex)
            );

            LoadMesh("Cube");
            LoadMesh("Sphere");
        }

        return false;
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

    MaterialID Graphics::LoadMaterial(std::string const& _name)
    {
        if (m_materialIDs.find(_name) != m_materialIDs.end())
            return m_materialIDs[_name];

        MaterialID material = m_currentMaterial;
        m_currentMaterial++;
        m_materialIDs[_name] = material;
        m_materials[material] = PBRMaterial{};

        return material;
    }

    MaterialID Graphics::CreateMaterial(std::string const& _name)
    {
        if (m_materialIDs.find(_name) != m_materialIDs.end())
            return MaterialID{ INVALID_ID };

        MaterialID material = m_currentMaterial;
        m_currentMaterial++;
        m_materialIDs[_name] = material;
        m_materials[material] = PBRMaterial{};

        return material;
    }

    MaterialID Graphics::CreateMaterial(std::string const& _name, MaterialID const& _id)
    {
        return -1;
    }

    InstanceID Graphics::CreateInstance(MeshID const& _mesh, MaterialID const& _material)
    {
        ObjectInstance instance{};
        instance.material = _material;
        instance.world = Matrix::Identity;
        instance.mesh = _mesh;
        
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

    bool Graphics::UpdateMaterial(MaterialID const& _id, PBRMaterial const& _material)
    {
        if (m_materials.find(_id) != m_materials.end())
        {
            m_materials[_id] = _material;
            return true;
        }
        return false;
    }

    VertexBuffer Graphics::CreateVertexBuffer(
        VertexCollection& _vertices,
        IndexCollection& _indices
    )
    {
        using namespace DirectX;
        VertexBuffer vb;
        ResourceUploadBatch uploadBatch(m_device.Get());
        uploadBatch.Begin();
        ThrowIfFailed(CreateStaticBuffer(
            m_device.Get(),
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
            m_device.Get(),
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

    std::shared_ptr<IRenderTarget> Graphics::CreateRenderTarget()
    {
        auto rt = std::make_shared<RenderTarget>(m_device, *m_resourceDescriptors, DXGI_FORMAT_R8G8B8A8_UNORM, Vector2{ 1920, 1080 });

        if (rt)
        {
            m_renderTargets.push_back(rt);
        }

        return rt;
    }

    std::shared_ptr<DepthTarget> Graphics::CreateDepthTarget()
    {
        std::shared_ptr<DepthTarget> md = std::make_shared<DepthTarget>();

        md->heap = std::make_unique<DescriptorHeap>(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, FRAME_COUNT);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            1920, 1080, 1U, 0U, 1U, 0U,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
        for (int i = 0; i < FRAME_COUNT; i++)
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

    HRESULT Graphics::FindAdapter(ComPtr<IDXGIAdapter>& _adapter)
    {
        HRESULT hr = S_OK;
        ComPtr<IDXGIFactory> factory;
        RIF(CreateDXGIFactory(IID_PPV_ARGS(&factory)), "Failed to Create DXGI Factory");

        int i = 0;
        ComPtr<IDXGIAdapter> adapter;
        SIZE_T currentBest = 0;
        ComPtr<IDXGIAdapter> bestAdapter;
        while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc{};
            adapter->GetDesc(&desc);
            std::wcout << desc.Description << std::endl;
            std::wcout << desc.DedicatedSystemMemory << std::endl;
            std::wcout << desc.DedicatedVideoMemory << std::endl;
            std::wcout << desc.SharedSystemMemory << std::endl;
            if (desc.DedicatedSystemMemory + desc.DedicatedVideoMemory + desc.SharedSystemMemory > currentBest)
            {
                currentBest = desc.DedicatedSystemMemory + desc.DedicatedVideoMemory + desc.SharedSystemMemory;
                bestAdapter = adapter;
            }
            i++;
        }

        _adapter = bestAdapter;

        ComPtr<IDXGIAdapter3> a;
        _adapter.As(&a);
        m_adapter = a;
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        a->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
        Log::Debug("Local Budget: " + std::to_string(info.Budget >> 30) + "GB");
        a->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info);
        Log::Debug("Non Local Budget: " + std::to_string(info.Budget >> 30) + "GB");

        return hr;
    }

    VOID CALLBACK Graphics::OnDeviceRemoved(PVOID _pContext, BOOLEAN)
    {
        Graphics* graphics = (Graphics*)_pContext;
        graphics->DeviceRemoved();
    }

    void Graphics::DeviceRemoved()
    {
        HRESULT removedReason = m_device->GetDeviceRemovedReason();
        ComPtr<ID3D12DeviceRemovedExtendedData> pDred;
        ThrowIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&pDred)));
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput{};
        D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput{};
        ThrowIfFailed(pDred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput));
        ThrowIfFailed(pDred->GetPageFaultAllocationOutput(&pageFaultOutput));
        throw DirectXException(removedReason);
    }

    void Graphics::RenderObjects()
    {
        auto& cmd = m_cmds[m_frameIndex];
        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        GraphicsResource lightsResource = m_graphicsMemory->Allocate(m_lights.size() * sizeof(Light));
        memcpy(lightsResource.Memory(), m_lights.data(), sizeof(Light) * m_lights.size());

        for (auto const& renderTarget : m_renderTargets)
        {
            cmd->OMSetRenderTargets(1, &renderTarget->GetRTV(), false, &m_depthTargets[0]->dsvs[m_depthTargets[0]->frameIndex]);
            cmd->RSSetViewports(1, &renderTarget->GetViewport());
            cmd->RSSetScissorRects(1, &renderTarget->GetScissor());

            ShaderConstants shaderConstants{};
            shaderConstants.viewProjection = renderTarget->GetView() * renderTarget->GetProjection();

            XMStoreFloat3A(&shaderConstants.cameraPosition, (XMVECTOR)renderTarget->GetCameraPosition());

            GraphicsResource shaderConstantsResource = m_graphicsMemory->AllocateConstant<ShaderConstants>(shaderConstants);

            VertexBuffer const& skyboxVb = m_meshes[m_meshIDs["Cube"]];
            cmd->SetGraphicsRootSignature(m_skyboxRootSignature.Get());
            cmd->SetPipelineState(m_skyboxPipeline.Get());

            cmd->IASetVertexBuffers(0, 1, &skyboxVb.vbv);
            cmd->IASetIndexBuffer(&skyboxVb.ibv);
            cmd->SetGraphicsRootConstantBufferView(0, shaderConstantsResource.GpuAddress());
            cmd->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(m_skyboxTextureIndex));
            cmd->DrawIndexedInstanced(skyboxVb.count, 1, 0, 0, 0);


            cmd->SetGraphicsRootSignature(m_objectRootSignature.Get());
            cmd->SetPipelineState(m_objectPipeline.Get());
            cmd->SetGraphicsRootConstantBufferView(0, shaderConstantsResource.GpuAddress());
            cmd->SetGraphicsRootConstantBufferView(1, lightsResource.GpuAddress());
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

                    GraphicsResource materialResource = m_graphicsMemory->AllocateConstant<PBRMaterial>();
                    PBRMaterial* material = (PBRMaterial*)materialResource.Memory();
                    *material = m_materials[instance.material];

                    cmd->SetGraphicsRootConstantBufferView(2, materialResource.GpuAddress());
                    cmd->SetGraphicsRootConstantBufferView(3, shaderObjectResource.GpuAddress());
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

        std::array heaps = { m_resourceDescriptors->Heap(), m_states->Heap() };

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
            bars.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                it->GetTexture().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
            ));
            it->IncrementFrameIndex();
        }
        cmd->ResourceBarrier(static_cast<UINT>(bars.size()), bars.data());
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
        m_frameIndex %= FRAME_COUNT;

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
        if (!UnregisterWait(m_waitHandle))
            Log::Error(GetLastError());
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
        static Graphics graphics{};
        return graphics;
    }
}
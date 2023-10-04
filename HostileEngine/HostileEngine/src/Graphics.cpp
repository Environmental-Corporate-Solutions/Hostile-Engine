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
    RenderContext::RenderContext(ComPtr<ID3D12Device> const& _device)
        : m_device(_device)
    {
        for (auto& it : m_cmds)
        {
            it.Init(m_device);
        }

        RenderTargetState rtState(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );

        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState
        );

        pd.depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(
            TRUE, 
            D3D12_DEPTH_WRITE_MASK_ALL,
            D3D12_COMPARISON_FUNC_LESS_EQUAL, 
            TRUE,
            0XFF, 0XFF, 
            D3D12_STENCIL_OP_KEEP, 
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_REPLACE, 
            D3D12_COMPARISON_FUNC_ALWAYS,

            D3D12_STENCIL_OP_KEEP, 
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_KEEP, 
            D3D12_COMPARISON_FUNC_ALWAYS
        );
        m_effect = std::make_shared<BasicEffect>(m_device.Get(), EffectFlags::PerPixelLighting | EffectFlags::Texture, pd);
        pd.depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;
        pd.depthStencilDesc.DepthEnable = FALSE;
        
        m_stencilEffect = std::make_shared<BasicEffect>(m_device.Get(), EffectFlags::None, pd);
        const std::array<D3D12_INPUT_ELEMENT_DESC, 7> InputElements =
        {
            D3D12_INPUT_ELEMENT_DESC{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "COLOR",       0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "BLENDINDICES",0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        D3D12_INPUT_LAYOUT_DESC inputLayout;
        inputLayout.NumElements        = InputElements.size();
        inputLayout.pInputElementDescs = InputElements.data();
        EffectPipelineStateDescription skinnedpd(
            &inputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullClockwise,
            rtState
        );
        m_skinnedEffect = std::make_shared<SkinnedEffect>(m_device.Get(), EffectFlags::PerPixelLighting, skinnedpd);
        m_skinnedEffect->EnableDefaultLighting();
    }

    void RenderContext::SetRenderTarget(
        std::shared_ptr<RenderTarget>& _rt,
        std::shared_ptr<DepthTarget>& _dt
    )
    {
        auto const& cmd = m_cmds[m_currentFrame].cmd;
        cmd->OMSetRenderTargets(1, &_rt->rtv[_rt->frameIndex], false, (_dt != nullptr) ? &_dt->dsvs[_dt->frameIndex] : nullptr);

        cmd->RSSetViewports(1, &_rt->vp);
        cmd->RSSetScissorRects(1, &_rt->scissor);
    }

    void RenderContext::RenderVertexBuffer(
        VertexBuffer const& _vertexBuffer,
        Matrix& _world
    )
    {
        auto const& cmd = m_cmds[m_currentFrame].cmd;
        m_effect->SetWorld(_world);
        m_effect->Apply(cmd.Get());
        cmd->IASetIndexBuffer(&_vertexBuffer.ibv);
        cmd->IASetVertexBuffers(0, 1, &_vertexBuffer.vbv);
        cmd->DrawIndexedInstanced(_vertexBuffer.count, 1, 0, 0, 0);
    }

    void RenderContext::RenderVertexBuffer(
        VertexBuffer const& _vb,
        GTexture const& _mt,
        std::vector<Matrix> const& _bones,
        Matrix const& _world
    )
    {
        auto const& cmd = m_cmds[m_currentFrame].cmd;
        m_skinnedEffect->SetWorld(_world);
        std::vector<XMMATRIX> bones;
        for (const auto& it : _bones)
        {
            bones.push_back(it);
        }
        m_skinnedEffect->SetTexture(
            _mt.srv,
            m_sampler
        );
        m_skinnedEffect->SetBoneTransforms(bones.data(), bones.size());
        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_skinnedEffect->Apply(cmd.Get());
        cmd->IASetIndexBuffer(&_vb.ibv);
        cmd->IASetVertexBuffers(0, 1, &_vb.vbv);
        cmd->DrawIndexedInstanced(_vb.count, 1, 0, 0, 0);
    }

    void RenderContext::RenderGeometricPrimitive(
        GeometricPrimitive const& _primitive, Matrix const& _world
    )
    {
        auto const& cmd = m_cmds[m_currentFrame].cmd;
        m_effect->SetWorld(_world);
        m_effect->SetTexture(
            m_nullDescriptor,
            m_sampler
        );
        m_effect->SetColorAndAlpha({ 1,1,1,1 });
        m_effect->SetDiffuseColor({ 1,1,1,1 });
        m_effect->SetSpecularColor({ 1,1,1,1 });
        m_effect->Apply(cmd.Get());
        cmd->OMSetStencilRef(0xFF);
        _primitive.Draw(cmd.Get());
        cmd->OMSetStencilRef(0x0F);
        m_stencilEffect->SetWorld(Matrix::CreateScale(1.1f) * _world);
        m_stencilEffect->SetColorAndAlpha({ 0, 1, 1, 1 });
        m_stencilEffect->Apply(cmd.Get());
        _primitive.Draw(cmd.Get());
    }

    void RenderContext::RenderGeometricPrimitive(
        GeometricPrimitive const& _primitive,
        GTexture const& _texture, Matrix const& _world
    )
    {
        auto const& cmd = m_cmds[m_currentFrame].cmd;
        m_effect->SetWorld(_world);
        m_effect->SetTexture(
            _texture.srv,
            m_sampler
        );
        m_effect->Apply(cmd.Get());
        
        _primitive.Draw(cmd.Get());
    }


    void RenderContext::Wait()
    {
        m_cmds[m_currentFrame].Wait();
    }

    MaterialFactory::MaterialFactory(ComPtr<ID3D12Device>& _device)
        : m_device(_device)
    {
    }

    std::shared_ptr<BasicEffect> MaterialFactory::CreateBasicEffect(std::string _name, uint32_t effectFlags, const EffectPipelineStateDescription& pipelineDescription)
    {
        if (m_basicEffects.find(_name) != m_basicEffects.end())
            return m_basicEffects[_name];

        auto s = std::make_shared<BasicEffect>(m_device.Get(), effectFlags, pipelineDescription);
        m_basicEffects.try_emplace(_name, s);
        return s;
    }

    std::shared_ptr<MaterialFactory::Effect> MaterialFactory::CreateCustomEffect(
        std::string _name, std::array<std::string, static_cast<UINT>(Shaders::COUNT)>& _shaders, EffectPipelineStateDescription const& _pipelineDesc
    )
    {
        if (m_customEffects.find(_name) != m_customEffects.end())
        //{
        //    return m_customEffects[_name];
        //}
        //
        //static std::vector<std::string> entryPoints = {
        //    "vs_5_1",
        //    "ps_5_1",
        //    "ds_5_1",
        //    "hs_5_1",
        //    "gs_5_1",
        //};
        //ComPtr<ID3D12
        //for (UINT i = 0; i < _shaders.size(); i++)
        //{
        //    std::string path = "Assets/Shaders/" + _name + ".hlsl";
        //    D3DCompileFromFile(
        //        ConvertToWideString(path).c_str(),
        //        nullptr,
        //        nullptr,
        //        "main",
        //        entryPoints[i].c_str(),
        //        D3DCOMPILE_DEBUG,
        //        0,
        //
        //    );
        //}
        return std::shared_ptr<Effect>();
    }

    Graphics::GRESULT Graphics::Init(GLFWwindow* _pWindow)
    {
        ComPtr<ID3D12Debug1> debug;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        debug->EnableDebugLayer();
        debug->SetEnableGPUBasedValidation(true);
        ComPtr<IDXGIAdapter> adapter;
        if (FAILED(FindAdapter(adapter)))
            return Graphics::G_FAIL;

        if (FAILED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
            return Graphics::G_FAIL;


        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));
        m_cmdQueue->SetName(L"Command Queue");
        HWND hwnd = glfwGetWin32Window(_pWindow);
        m_hwnd = hwnd;
        if (FAILED(m_swapChain.Init(m_device, adapter, m_cmdQueue, _pWindow)))
            return Graphics::G_FAIL;


        if (FAILED(m_pipeline.Read(m_device, "default_no_depth")))
            return Graphics::G_FAIL;


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

        m_renderTargetDescriptors = std::make_unique<DescriptorPile>(
            m_device.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            MAX_RENDER_TARGETS * FRAME_COUNT
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
            DXGI_FORMAT_D32_FLOAT
        );
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            sceneState
        );


        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        
        m_renderContexts.push_back(std::make_shared<RenderContext>(m_device));
        auto in = m_resourceDescriptors->Allocate();
        m_device->CreateShaderResourceView(
            nullptr,
            &srvDesc,
            m_resourceDescriptors->GetCpuHandle(in)
        );
        m_renderContexts[0]->m_nullDescriptor = m_resourceDescriptors->GetGpuHandle(in);
        return GRESULT::G_OK;
    }

    void Graphics::RenderDebug(Matrix& _mat)
    {
        
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

    std::unique_ptr<VertexBuffer> Graphics::CreateVertexBuffer(
        std::vector<VertexPositionNormalTangentColorTextureSkinning>& _vertices,
        std::vector<uint16_t>& _indices
    )
    {
        auto vb = std::make_unique<VertexBuffer>();
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
        vb->vbv.SizeInBytes = static_cast<UINT>(_vertices.size() * sizeof(VertexPositionNormalTangentColorTextureSkinning));
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
        vb->ibv.SizeInBytes = static_cast<UINT>(_indices.size() * sizeof(uint16_t));

        vb->count = static_cast<UINT>(_indices.size());
        return vb;
    }

    std::shared_ptr<RenderTarget> Graphics::CreateRenderTarget()
    {
        auto rt = std::make_shared<RenderTarget>();

        CD3DX12_RESOURCE_DESC rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        rtDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        size_t end;
        m_renderTargetDescriptors->AllocateRange(FRAME_COUNT, rt->rtvIndex, end);
        m_resourceDescriptors->AllocateRange(FRAME_COUNT, rt->srvIndex, end);
        for (int i = 0; i < FRAME_COUNT; i++)
        {
            HRESULT hr = m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
                &rtDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&rt->texture[i]));
            if (FAILED(hr))
            {
                Log::Error("Failed To Create Render Target: " + std::to_string(hr));
                return nullptr;
            }

            m_device->CreateRenderTargetView(
                rt->texture[i].Get(),
                nullptr,
                m_renderTargetDescriptors->GetCpuHandle(rt->rtvIndex + i)
            );

            m_device->CreateShaderResourceView(
                rt->texture[i].Get(),
                nullptr,
                m_resourceDescriptors->GetCpuHandle(rt->srvIndex + i)
            );

            rt->rtv[i] = m_renderTargetDescriptors->GetCpuHandle(rt->rtvIndex + i);
            rt->srv[i] = m_resourceDescriptors->GetGpuHandle(rt->srvIndex + i);
            rt->frameIndex = m_frameIndex;
            rt->vp.MaxDepth = 1;
            rt->vp.Height = 1080;
            rt->vp.Width = 1920;
            rt->scissor = D3D12_RECT{};
            rt->scissor.right = 1920;
            rt->scissor.bottom = 1080;
            rt->currentState[i] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        }

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

    /*void Graphics::SetRenderTarget(std::shared_ptr<MoltenRenderTarget>& _rt)
    {
        auto& cmd = m_directPipeline.GetCmd(m_frameIndex);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_directPipeline.m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_directPipeline.m_dsvHeapIncrementSize);
        cmd->OMSetRenderTargets(1, &_rt->rtv[m_frameIndex], true, &dsvHandle);
    }

    void Graphics::SetCamera(Matrix&& _view)
    {
        m_effect->SetView(_view);
        m_textureEffect->SetView(_view);
        m_skinnedEffect->SetView(_view);
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

    void Graphics::RenderGeometricPrimitive(
        std::unique_ptr<GeometricPrimitive>& _primitive,
        std::unique_ptr<MoltenTexture>& _texture,
        Matrix& _world
    )
    {
        auto& cmd = m_directPipeline.GetCmd(m_frameIndex);
        m_textureEffect->SetTexture(
            m_resourceDescriptors->GetGpuHandle(_texture->index),
            m_states->AnisotropicWrap()
        );
        m_textureEffect->SetDiffuseColor({ 1,1,1,1 });
        m_textureEffect->SetColorAndAlpha({ 1, 1, 1, 1 });
        m_textureEffect->SetWorld(_world);
        m_textureEffect->Apply(*cmd);

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
    }*/



    std::unique_ptr<GTexture> Graphics::CreateTexture(std::string const&& _name)
    {
        auto hr = S_OK;
        auto texture = std::make_unique<GTexture>();
        std::string path = "Assets/textures/" + _name + ".png";

        ResourceUploadBatch resourceUpload(m_device.Get());
        resourceUpload.Begin();

        hr = CreateWICTextureFromFile(
            m_device.Get(),
            resourceUpload,
            ConvertToWideString(path).c_str(),
            &texture->tex
        );
        if (FAILED(hr))
        {
            Log::Error("Failed to Create Texture (" + _name + ")\n ErrorCode: " + std::to_string(hr));
            return nullptr;
        }

        auto finished = resourceUpload.End(m_cmdQueue.Get());
        finished.wait();

        size_t index = m_resourceDescriptors->Allocate();

        m_device->CreateShaderResourceView(
            texture->tex.Get(),
            nullptr,
            m_resourceDescriptors->GetCpuHandle(index)
        );
        texture->srv = m_resourceDescriptors->GetGpuHandle(index);
        return texture;
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

    void Graphics::BeginFrame()
    {
        CommandList& cmd = m_cmds[m_frameIndex];
        cmd.Wait();

        cmd.Reset(m_pipeline.m_pipeline);

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

        cmd->SetPipelineState(m_pipeline.m_pipeline.Get());
        cmd->SetGraphicsRootSignature(m_pipeline.m_rootSig.Get());
        cmd->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->RSSetViewports(1, &m_swapChain.m_viewport);
        cmd->RSSetScissorRects(1, &m_swapChain.m_scissorRect);

        cmd->ClearRenderTargetView(rtv, color.data(), 0, nullptr);



        D3D12_CLEAR_VALUE clearColor{};
        std::vector<D3D12_RESOURCE_BARRIER> bars;
        for (auto const& it : m_renderTargets)
        {
            if (it->currentState[it->frameIndex] == D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
            {
                bars.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                    it->texture[it->frameIndex].Get(),
                    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET
                ));
                it->currentState[it->frameIndex] = D3D12_RESOURCE_STATE_RENDER_TARGET;
            }
        }
        cmd->ResourceBarrier(static_cast<UINT>(bars.size()), bars.data());

        for (auto const& it : m_renderTargets)
        {
            cmd->ClearRenderTargetView(it->rtv[it->frameIndex], clearColor.Color, 0, nullptr);
        }
        for (auto const& it : m_depthTargets)
        {
            it->frameIndex = m_frameIndex;
            cmd->ClearDepthStencilView(it->dsvs[it->frameIndex], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0x0, 0, nullptr);
        }

        std::array heaps = { m_resourceDescriptors->Heap(), m_states->Heap() };
        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

        for (auto const& it : m_renderContexts)
        {
            it->m_currentFrame = m_frameIndex;
            it->m_sampler = m_states->AnisotropicWrap();
            it->Wait();
            it->m_cmds[it->m_currentFrame].allocator->Reset();
            it->m_cmds[it->m_currentFrame].cmd->Reset(it->m_cmds[it->m_currentFrame].allocator.Get(), nullptr);
            it->m_cmds[it->m_currentFrame].cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
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
        //m_cmds[m_frameIndex]->SetDescriptorHeaps(1, m_imGuiHeap.GetAddressOf());
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *m_cmds[m_frameIndex]);
    }

    void Graphics::EndFrame()
    {

        CommandList& cmd = m_cmds[m_frameIndex];
        cmd->Reset(cmd.allocator.Get(), m_pipeline.m_pipeline.Get());

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetBackBuffer(m_frameIndex);
        cmd->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
        std::array heaps = { m_resourceDescriptors->Heap(), m_states->Heap() };
        cmd->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
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
                it->texture[it->frameIndex].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
            ));
            it->currentState[it->frameIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            it->frameIndex = (it->frameIndex + 1) % FRAME_COUNT;
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
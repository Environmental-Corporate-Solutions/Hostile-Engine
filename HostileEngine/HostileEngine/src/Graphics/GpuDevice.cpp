#include "stdafx.h"
#include "GpuDevice.h"

#include <d3d12shader.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>

namespace Hostile
{
    void GpuDevice::Init()
    {
#ifdef _DEBUG
        ComPtr<ID3D12Debug1> debug;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        debug->EnableDebugLayer();
        debug->SetEnableGPUBasedValidation(true);

        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings)));
        pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif

        FindAdapter();

        ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

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

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));

        //m_graphicsMemory = std::make_unique<GraphicsMemory>(m_device.Get());
        m_resourceDescriptors = std::make_unique<DescriptorPile>(
            m_device.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            1024
        );
        //m_states = std::make_unique<CommonStates>(m_device.Get());
    }

    GpuDevice::~GpuDevice()
    {
        if (!UnregisterWait(m_waitHandle))
            Log::Error(GetLastError());
        CloseHandle(m_deviceRemovedEvent);
    }

    ComPtr<ID3D12Device> GpuDevice::Device()
    {
        return m_device.Get();
    }

    ComPtr<IDXGIAdapter3> GpuDevice::Adapter()
    {
        return m_adapter.Get();
    }

    ID3D12Device* GpuDevice::operator->()
    {
        return m_device.Get();
    }

    DescriptorPile& GpuDevice::ResourceHeap()
    {
        return *m_resourceDescriptors;
    }

    void GpuDevice::LoadTexture(std::string _name, Texture& _texture)
    {
        auto& it = m_textures.find(_name);
        if (it != m_textures.end())
        {
            _texture = it->second;
            return;
        }
        ResourceUploadBatch uploadBatch(m_device.Get());
        uploadBatch.Begin();
        ThrowIfFailed(CreateWICTextureFromFile(m_device.Get(), uploadBatch, ConvertToWideString("Assets/textures/" + _name).c_str(), &_texture.texture));
        _texture.index = m_resourceDescriptors->Allocate();
        m_device->CreateShaderResourceView(
            _texture.texture.Get(),
            nullptr,
            m_resourceDescriptors->GetCpuHandle(_texture.index)
        );
        _texture.name = _name;
        uploadBatch.End(m_queue.Get()).wait();
        m_textures.emplace(_name, _texture);
    }

    //Pipeline GpuDevice::LoadPipeline(std::string& _name)
    //{
    //    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    //   
    //    return Pipeline();
    //}

    void GpuDevice::FindAdapter()
    {
        HRESULT hr = S_OK;
        ComPtr<IDXGIFactory> factory;
        ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&factory)));

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

        ComPtr<IDXGIAdapter3> a;
        bestAdapter.As(&a);
        m_adapter = a;
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        a->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
        Log::Debug("Local Budget: " + std::to_string(info.Budget >> 30) + "GB");
        a->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info);
        Log::Debug("Non Local Budget: " + std::to_string(info.Budget >> 30) + "GB");
    }

    VOID GpuDevice::OnDeviceRemoved(PVOID _pContext, BOOLEAN)
    {
        GpuDevice* pDevice = (GpuDevice*)_pContext;
        pDevice->DeviceRemoved();
    }

    void GpuDevice::DeviceRemoved()
    {
        HRESULT removedReason = m_device->GetDeviceRemovedReason();
#ifdef _DEBUG
        ComPtr<ID3D12DeviceRemovedExtendedData> pDred;
        ThrowIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&pDred)));
        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput{};
        D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput{};
        ThrowIfFailed(pDred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput));
        ThrowIfFailed(pDred->GetPageFaultAllocationOutput(&pageFaultOutput));
#endif
        throw DirectXException(removedReason);
    }
}
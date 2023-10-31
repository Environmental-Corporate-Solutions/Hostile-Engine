#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "GraphicsHelpers.h"
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/CommonStates.h>
#include <memory>
#include "GraphicsTypes.h"

namespace Hostile
{
    using namespace Microsoft::WRL;
    using namespace DirectX;
    class GpuDevice
    {
    public:
        void Init();
        ~GpuDevice();
        ComPtr<ID3D12Device> Device();
        ComPtr<IDXGIAdapter3> Adapter();
        ComPtr<ID3D12CommandQueue> Queue();
        ComPtr<ID3D12CommandQueue> CopyQueue();
        ID3D12Device* operator->();
        DescriptorPile& ResourceHeap();

        void LoadTexture(std::string _name, Texture& _texture);
        //Pipeline LoadPipeline(std::string& _name);
    private:
        void FindAdapter();
        static VOID CALLBACK OnDeviceRemoved(PVOID _pContext, BOOLEAN);
        void DeviceRemoved();

    private:
        ComPtr<ID3D12Device> m_device;
        ComPtr<IDXGIAdapter3> m_adapter;
        ComPtr<ID3D12CommandQueue> m_queue;
        ComPtr<ID3D12CommandQueue> m_copy_queue;

        std::unique_ptr<CommonStates>   m_states = nullptr;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory = nullptr;
        std::unique_ptr<DescriptorPile> m_resourceDescriptors = nullptr;

        std::unordered_map<std::string, Texture> m_textures;

        ComPtr<ID3D12Fence> m_deviceFence;
        HANDLE m_deviceRemovedEvent = nullptr;
        HANDLE m_waitHandle = nullptr;
    };
}



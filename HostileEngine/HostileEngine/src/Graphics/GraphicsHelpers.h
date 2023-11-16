#pragma once
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>
#include <vector>
#include "DirectXTK12/SimpleMath.h"
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <vector>
#include <iostream>  
#include <d3dx12.h>
#include <d3dcompiler.h>
#include "GLFW/glfw3.h"    
#include "GLFW/glfw3native.h"

#include "toml++/toml.h"

#include <codecvt>

constexpr UINT g_frame_count = 2;
#define RIF(x, y) hr = x; if (FAILED(hr)) {std::cerr << y << std::endl << "Error: " << hr << std::endl; return hr;}

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace Hostile
{
    class DirectXException : public std::exception
    {
    public:
        explicit DirectXException(HRESULT _error)
            : error_(_error) {}

        ~DirectXException() noexcept final = default;

        const char* what() const noexcept final
        {
            return msg_;
        }

        HRESULT error() const noexcept
        {
            return error_;
        }

    private:
        static constexpr const char* msg_ = "DirectX Error: ";
        HRESULT error_;
    };

    inline void ThrowIfFailed(HRESULT _hr)
    {
        if (FAILED(_hr))
            throw DirectXException(_hr);
    }

    std::wstring ConvertToWideString(std::string const& _str);
    DXGI_FORMAT FormatFromString(const std::string& _str);

    struct CommandList
    {
        ComPtr<ID3D12GraphicsCommandList> cmd;
        ComPtr<ID3D12CommandAllocator> allocator;

        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent;
        UINT m_fenceValue = 0;

        HRESULT Init(ComPtr<ID3D12Device> const& _device)
        {
            HRESULT hr = S_OK;
            RIF(_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&allocator)
            ), "Failed to Create Command Allocator");

            RIF(_device->CreateCommandList(
                0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                allocator.Get(),
                nullptr,
                IID_PPV_ARGS(&cmd)
            ), "Failed to create Command List");
            cmd->Close();

            RIF(
                _device->CreateFence(
                    m_fenceValue,
                    D3D12_FENCE_FLAG_NONE,
                    IID_PPV_ARGS(&m_fence)
                ),
                "Failed to Create Fence"
            );

            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            return hr;
        }
        HRESULT Reset(ComPtr<ID3D12PipelineState> _pipeline) const
        {
            allocator->Reset();
            return cmd->Reset(allocator.Get(), (_pipeline != nullptr) ? _pipeline.Get() : nullptr);
        }

        bool IsInFlight() const
        {
            return (m_fence->GetCompletedValue() < m_fenceValue) && (m_fence->GetCompletedValue() != UINT64_MAX);
        }

        void Wait()
        {
            if (IsInFlight())
            {
                m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }
        }

        void Shutdown()
        {
            Wait();
            allocator->Reset();

            cmd.Reset();
            allocator.Reset();
            CloseHandle(m_fenceEvent);
        }
        ID3D12GraphicsCommandList* operator->()
        {
            return cmd.Get();
        }

        ID3D12GraphicsCommandList* operator*()
        {
            return cmd.Get();
        }
    };

    struct SwapChain
    {
        ComPtr<IDXGISwapChain3> swapChain;
        std::array<ComPtr<ID3D12Resource>, g_frame_count> rtvs;
        ComPtr<ID3D12DescriptorHeap> rtvHeap;
        UINT rtvDescriptorSize;
        D3D12_VIEWPORT m_viewport;
        D3D12_RECT m_scissorRect;
        DXGI_FORMAT m_format;

        HRESULT Init(ComPtr<ID3D12Device>& _device, ComPtr<IDXGIAdapter3>& _adapter, ComPtr<ID3D12CommandQueue>& _cmdQueue, GLFWwindow* _pWindow)
        {
            HRESULT hr = S_OK;

            ComPtr<IDXGIFactory2> factory;
            RIF(_adapter->GetParent(IID_PPV_ARGS(&factory)), "Failed to Get Factory");


            int width = 0;
            int height = 0;
            glfwGetWindowSize(_pWindow, &width, &height);
            m_viewport.Width    = (FLOAT)width;
            m_viewport.Height   = (FLOAT)height;
            m_viewport.MaxDepth = 1;
            m_viewport.MinDepth = 0;
            m_viewport.TopLeftX = 0;
            m_viewport.TopLeftY = 0;

            m_scissorRect.left   = 0;
            m_scissorRect.right  = (LONG)width;
            m_scissorRect.top    = 0;
            m_scissorRect.bottom = (LONG)height;

            DXGI_SWAP_CHAIN_DESC1 scDesc{};

            scDesc.Width              = static_cast<UINT>(m_viewport.Width);
            scDesc.Height             = static_cast<UINT>(m_viewport.Height);
            scDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
            scDesc.Stereo             = false;
            scDesc.SampleDesc.Quality = 0;
            scDesc.SampleDesc.Count   = 1;
            scDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scDesc.BufferCount        = 2;
            scDesc.Scaling            = DXGI_SCALING_STRETCH;
            scDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            scDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
            scDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            m_format                  = DXGI_FORMAT_R8G8B8A8_UNORM;

            UINT refresh_rate = 60;
            DEVMODE dev_mode;
            ZeroMemory(&dev_mode, sizeof(dev_mode));
            if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
            {
                refresh_rate = static_cast<UINT>(dev_mode.dmDisplayFrequency);
            }
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfDesc{};
            scfDesc.RefreshRate.Numerator   = refresh_rate;
            scfDesc.RefreshRate.Denominator = 1;
            scfDesc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
            scfDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
            scfDesc.Windowed                = true;
            HWND hwnd                       = glfwGetWin32Window(_pWindow);
            ComPtr<IDXGISwapChain1> sc;
            RIF(factory->CreateSwapChainForHwnd(_cmdQueue.Get(), hwnd, &scDesc, &scfDesc, nullptr, &sc), "Failed to Create SwapChain");

            sc.As(&swapChain);
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
            rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.NumDescriptors = g_frame_count;

            RIF(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)), "Failed to Create RTV Descriptor Heap");
            rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            CreateBuffers();

            return hr;
        }

        HRESULT CreateBuffers()
        {
            HRESULT hr = S_OK;
            ComPtr<ID3D12Device> device;
            swapChain->GetDevice(IID_PPV_ARGS(&device));
            for (int i = 0; i < g_frame_count; i++)
            {
                RIF(swapChain->GetBuffer(i, IID_PPV_ARGS(&rtvs[i])), "Failed to Get Buffer");
                rtvs[i]->SetName(std::wstring(L"RenderTarget " + std::to_wstring(i)).c_str());
                CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
                rtvHandle.Offset(i, rtvDescriptorSize);
                device->CreateRenderTargetView(rtvs[i].Get(), nullptr, rtvHandle);
            }
            return hr;
        }

        HRESULT Resize(UINT _width, UINT _height)
        {
            for (int i = 0; i < g_frame_count; i++)
            {
                rtvs[i] = nullptr;
            }
            HRESULT hr = S_OK;
            m_viewport.Width     = static_cast<FLOAT>(_width);
            m_viewport.Height    = static_cast<FLOAT>(_height);
            m_scissorRect.right  = static_cast<LONG>(_width);
            m_scissorRect.bottom = static_cast<LONG>(_height);
            RIF(
                swapChain->ResizeBuffers(g_frame_count, _width, _height, m_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH),
                "Failed to Resize SwapChain"
            );
            CreateBuffers();
            return hr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetBackBuffer(size_t _frameIndex)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
            rtvHandle.Offset(swapChain->GetCurrentBackBufferIndex(), rtvDescriptorSize);
            return rtvHandle;
        }
    };

    
}
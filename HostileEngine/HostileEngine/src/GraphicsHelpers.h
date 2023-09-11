#pragma once
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>
#include <vector>
#include "DirectXTK/SimpleMath.h"
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

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")

#define FRAME_COUNT 2
#define RIF(x, y) hr = x; if (FAILED(hr)) { std::cerr << y << std::endl << "Error: " << hr << std::endl; return hr; }

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

struct Vertex
{
    Vector4 pos;
    Vector4 normal;
};

struct VertexBuffer
{
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    size_t vertexCount;

    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibView;
};

struct Texture
{
    ComPtr<ID3D12Resource> texture;
};

struct CommandList
{
    ComPtr<ID3D12GraphicsCommandList> cmd;
    ComPtr<ID3D12CommandAllocator> allocator;

    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent;
    UINT m_fenceValue = 0;

    HRESULT Init(ComPtr<ID3D12Device>& _device)
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

        m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    HRESULT Reset(ComPtr<ID3D12PipelineState> _pipeline)
    {
        allocator->Reset();
        return cmd->Reset(allocator.Get(), _pipeline.Get());
    }

    bool IsInFlight()
    {
        return m_fence->GetCompletedValue() < m_fenceValue;
    }

    void Wait()
    {
        if (IsInFlight())
        {
            m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
            //m_fenceValue++;
        }
        //m_fenceValue++;
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
    ComPtr<ID3D12Resource> rtvs[FRAME_COUNT];
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    UINT rtvDescriptorSize;
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    DXGI_FORMAT m_format;

    HRESULT Init(ComPtr<ID3D12Device>& _device, ComPtr<IDXGIAdapter>& _adapter, ComPtr<ID3D12CommandQueue>& _cmdQueue, GLFWwindow* _pWindow)
    {
        HRESULT hr = S_OK;

        //ComPtr<IDXGIDevice> dxgiDevice;
        //RIF(_device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)), "Failed to Query Interface for DXGIDevice")
        //ComPtr<IDXGIAdapter> adapter;
        //RIF(dxgiDevice->GetAdapter(&adapter), "Failed to Get Adapter");
        ComPtr<IDXGIFactory2> factory;
        RIF(_adapter->GetParent(IID_PPV_ARGS(&factory)), "Failed to Get Factory");


        //RECT r{};
        //GetWindowRect(_hwnd, &r);
        int width, height;
        glfwGetWindowSize(_pWindow, &width, &height);
        m_viewport.Width = (FLOAT)width;
        m_viewport.Height = (FLOAT)height;
        m_viewport.MaxDepth = 1;
        m_viewport.MinDepth = 0;
        m_viewport.TopLeftX = 0;
        m_viewport.TopLeftY = 0;

        m_scissorRect.left = 0;
        m_scissorRect.right = width;
        m_scissorRect.top = 0;
        m_scissorRect.bottom = height;

        DXGI_SWAP_CHAIN_DESC1 scDesc{};

        scDesc.Width = m_viewport.Width;
        scDesc.Height = m_viewport.Height;
        scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.Stereo = false;
        scDesc.SampleDesc.Quality = 0;
        scDesc.SampleDesc.Count = 1;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.BufferCount = 2;
        scDesc.Scaling = DXGI_SCALING_STRETCH;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfDesc{};
        scfDesc.RefreshRate.Numerator = 60;
        scfDesc.RefreshRate.Denominator = 1;
        scfDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
        scfDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
        scfDesc.Windowed = true;
        HWND hwnd = glfwGetWin32Window(_pWindow);
        ComPtr<IDXGISwapChain1> sc;
        RIF(factory->CreateSwapChainForHwnd(_cmdQueue.Get(), hwnd, &scDesc, &scfDesc, nullptr, &sc), "Failed to Create SwapChain");

        sc.As(&swapChain);
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = FRAME_COUNT;

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
        for (int i = 0; i < FRAME_COUNT; i++)
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
        for (int i = 0; i < FRAME_COUNT; i++)
        {
            rtvs[i] = nullptr;
        }
        HRESULT hr = S_OK;
        m_viewport.Width = _width;
        m_viewport.Height = _height;
        m_scissorRect.right = _width;
        m_scissorRect.bottom = _height;
        RIF(
            swapChain->ResizeBuffers(FRAME_COUNT, _width, _height, m_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH),
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

struct Pipeline
{
    ComPtr<ID3D12PipelineState> m_pipeline;
    ComPtr<ID3D12RootSignature> m_rootSig;

    HRESULT Init(
        ComPtr<ID3D12Device>& _device,
        CD3DX12_DEPTH_STENCIL_DESC _depthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        DXGI_FORMAT _depthStencilFormat = DXGI_FORMAT_D32_FLOAT
    )
    {
        HRESULT hr = S_OK;
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        ComPtr<ID3DBlob> error;
        RIF(D3DCompileFromFile(
            L"Shaders/VertexShader.hlsl",
            nullptr, nullptr,
            "main", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
            0, &vertexShader, &error
        ), reinterpret_cast<char*>(error->GetBufferPointer()));

        RIF(D3DCompileFromFile(
            L"Shaders/PixelShader.hlsl",
            nullptr, nullptr,
            "main", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
            0, &pixelShader, &error
        ), error->GetBufferPointer());

        RIF(_device->CreateRootSignature(0, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)), "Failed to create RootSig");
        D3D12_INPUT_ELEMENT_DESC inputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, }
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElements, _countof(inputElements) };
        psoDesc.pRootSignature = m_rootSig.Get();
        psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
        psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = _depthStencilFormat;
        psoDesc.DepthStencilState = _depthStencil;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        RIF(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipeline)), "Failed to Create Pipeline");

        return hr;
    }
};
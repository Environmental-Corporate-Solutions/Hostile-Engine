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

#define FRAME_COUNT 2
#define RIF(x, y) hr = x; if (FAILED(hr)) { std::cerr << y << std::endl << "Error: " << hr << std::endl; return hr; }

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

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
    std::string m_name;

    HRESULT Read(ComPtr<ID3D12Device>& _device, std::string _name)
    {
        HRESULT hr = S_OK;
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        

        try
        {
            auto materialData = toml::parse_file("Assets/pipelines/" + _name + ".toml");
            m_name = materialData["Name"].value_or("Pipeline");
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> error;
            // Compile Vertex Shader and Root Signature
            {
                std::string_view name = materialData["Shaders"]["vertex"].value_or("VertexShader.hlsl");
                RIF(
                    D3DCompileFromFile(
                        converter.from_bytes(std::string("Assets/Shaders/") + name.data()).c_str(),
                        nullptr, nullptr,
                        "main", "vs_5_0",
                        D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
                        0, &vertexShader, &error
                    ),
                    reinterpret_cast<char*>(error->GetBufferPointer())
                );

                RIF(
                    _device->CreateRootSignature(
                        0,
                        vertexShader->GetBufferPointer(),
                        vertexShader->GetBufferSize(),
                        IID_PPV_ARGS(&m_rootSig)
                    ),
                    "Failed to create RootSig"
                );
            }
            std::cout << "after vertex shader" << std::endl;
            ComPtr<ID3DBlob> pixelShader;
            {
                std::string_view name = materialData["Shaders"]["pixel"].value_or("PixelShader.hlsl");

                RIF(
                    D3DCompileFromFile(
                        converter.from_bytes(std::string("Assets/Shaders/") + name.data()).c_str(),
                        nullptr, nullptr,
                        "main", "ps_5_0",
                        D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
                        0, &pixelShader, &error
                    ),
                    reinterpret_cast<char*>(error->GetBufferPointer())
                );
            }
            std::cout << "after pixel shader" << std::endl;


            D3D12_INPUT_ELEMENT_DESC inputElements[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "BONEID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "BONEID", 1, DXGI_FORMAT_R32G32B32A32_UINT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, },
                { "WEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0, }
            };

            CD3DX12_RASTERIZER_DESC rasterizer{};
            rasterizer.FillMode = (D3D12_FILL_MODE)materialData["Rasterizer"]["FillMode"].value_or((size_t)D3D12_FILL_MODE_SOLID);
            rasterizer.CullMode = (D3D12_CULL_MODE)materialData["Rasterizer"]["CullMode"].value_or((size_t)D3D12_CULL_MODE_BACK);
            rasterizer.FrontCounterClockwise = materialData["Rasterizer"]["FrontCounterClockwise"].value_or(TRUE);
            rasterizer.DepthBiasClamp = materialData["Rasterizer"]["DepthBiasClamp"].value_or(D3D12_DEFAULT_DEPTH_BIAS_CLAMP);
            rasterizer.SlopeScaledDepthBias = materialData["Rasterizer"]["SlopScaledDepthBias"].value_or(D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS);
            rasterizer.DepthClipEnable = materialData["Rasterizer"]["DepthClipEnable"].value_or(TRUE);
            rasterizer.MultisampleEnable = materialData["Rasterizer"]["MultisampleEnable"].value_or(FALSE);
            rasterizer.AntialiasedLineEnable = materialData["Rasterizer"]["AntialiasedLineEnable"].value_or(FALSE);
            rasterizer.ForcedSampleCount = materialData["Rasterizer"]["ForcedSampleCount"].value_or(0);
            rasterizer.ConservativeRaster = (D3D12_CONSERVATIVE_RASTERIZATION_MODE)materialData["Rasterizer"]["ConservativeRaster"].value_or((size_t)D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);

            CD3DX12_DEPTH_STENCIL_DESC dsDesc = {};

            dsDesc.DepthEnable = materialData["DepthStencil"]["DepthEnable"].value_or<BOOL>(TRUE);
            dsDesc.DepthWriteMask = (D3D12_DEPTH_WRITE_MASK)materialData["DepthStencil"]["DepthWriteMask"].value_or((size_t)D3D12_DEPTH_WRITE_MASK_ALL);
            dsDesc.DepthFunc = (D3D12_COMPARISON_FUNC)materialData["DepthStencil"]["DepthFunc"].value_or((size_t)D3D12_COMPARISON_FUNC_LESS);
            dsDesc.StencilEnable = materialData["DepthStencil"]["StencilEnable"].value_or<BOOL>(FALSE);
            dsDesc.StencilReadMask = materialData["DepthStencil"]["StencilReadMask"].value_or<UINT8>(D3D12_DEFAULT_STENCIL_READ_MASK);
            dsDesc.StencilWriteMask = materialData["DepthStencil"]["StencilWriteMask"].value_or<UINT8>(D3D12_DEFAULT_STENCIL_WRITE_MASK);

            D3D12_DEPTH_STENCILOP_DESC frontStencilOp = {};

            frontStencilOp.StencilFailOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["FrontStencilOp"]["StencilFailOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            frontStencilOp.StencilDepthFailOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["FrontStencilOp"]["StencilDepthFailOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            frontStencilOp.StencilPassOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["FrontStencilOp"]["StencilPassOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            frontStencilOp.StencilFunc = (D3D12_COMPARISON_FUNC)materialData["DepthStencil"]["FrontStencilOp"]["StencilFunc"].value_or((size_t)D3D12_COMPARISON_FUNC_ALWAYS);

            dsDesc.FrontFace = frontStencilOp;

            D3D12_DEPTH_STENCILOP_DESC backStencilOp = {};

            backStencilOp.StencilFailOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["BackStencilOp"]["StencilFailOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            backStencilOp.StencilDepthFailOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["BackStencilOp"]["StencilDepthFailOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            backStencilOp.StencilPassOp = (D3D12_STENCIL_OP)materialData["DepthStencil"]["BackStencilOp"]["StencilPassOp"].value_or((size_t)D3D12_STENCIL_OP_KEEP);
            backStencilOp.StencilFunc = (D3D12_COMPARISON_FUNC)materialData["DepthStencil"]["BackStencilOp"]["StencilFunc"].value_or((size_t)D3D12_COMPARISON_FUNC_ALWAYS);

            dsDesc.BackFace = backStencilOp;

            CD3DX12_BLEND_DESC blendDesc{};
            blendDesc.AlphaToCoverageEnable = false;
            blendDesc.IndependentBlendEnable = false;
            blendDesc.RenderTarget[0] = {
                FALSE,FALSE,
                D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL,
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_rootSig.Get();
            psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
            psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
            {
                ComPtr<ID3DBlob> domainShader;
                std::string_view name = materialData["Shaders"]["domain"].value_or("none");
                if (name != "none")
                {
                    RIF(
                        D3DCompileFromFile(
                            converter.from_bytes(std::string("Assets/Shaders/") + name.data()).c_str(),
                            nullptr, nullptr,
                            "main", "ds_5_0",
                            D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
                            0, &pixelShader, &error
                        ),
                        reinterpret_cast<char*>(error->GetBufferPointer())
                    );
                    psoDesc.DS = { reinterpret_cast<UINT8*>(domainShader->GetBufferSize()), domainShader->GetBufferSize() };
                }
            }
            {
                ComPtr<ID3DBlob> hullShader;
                std::string_view name = materialData["Shaders"]["hull"].value_or("none");
                if (name != "none")
                {
                    RIF(
                        D3DCompileFromFile(
                            converter.from_bytes(std::string("Assets/Shaders/") + name.data()).c_str(),
                            nullptr, nullptr,
                            "main", "hs_5_0",
                            D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
                            0, &pixelShader, &error
                        ),
                        reinterpret_cast<char*>(error->GetBufferPointer())
                    );
                    psoDesc.HS = { reinterpret_cast<UINT8*>(hullShader->GetBufferSize()), hullShader->GetBufferSize() };
                }
            }
            {
                ComPtr<ID3DBlob> geometryShader;
                std::string_view name = materialData["Shaders"]["geometry"].value_or("none");
                if (name != "none")
                {
                    RIF(
                        D3DCompileFromFile(
                            converter.from_bytes(std::string("Assets/Shaders/") + name.data()).c_str(),
                            nullptr, nullptr,
                            "main", "gs_5_0",
                            D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS,
                            0, &pixelShader, &error
                        ),
                        reinterpret_cast<char*>(error->GetBufferPointer())
                    );
                    psoDesc.GS = { reinterpret_cast<UINT8*>(geometryShader->GetBufferSize()), geometryShader->GetBufferSize() };
                }
            }
            std::cout << "after shaders" << std::endl;

            psoDesc.BlendState = blendDesc;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.RasterizerState = rasterizer;
            psoDesc.DepthStencilState = dsDesc;
            psoDesc.InputLayout = { inputElements, _countof(inputElements) };
            psoDesc.IBStripCutValue = (D3D12_INDEX_BUFFER_STRIP_CUT_VALUE)materialData["IBStripCutValue"].value_or((size_t)D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
            psoDesc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)materialData["PrimitiveTopologyType"].value_or((size_t)D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
            psoDesc.NumRenderTargets = materialData["NumRenderTargets"].value_or(1);
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            psoDesc.SampleDesc = { 1, 0 };
            psoDesc.NodeMask = 0;
            psoDesc.CachedPSO = { nullptr, 0 };
            psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
            RIF(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipeline)), "Failed to Create Pipeline");
            m_pipeline->SetName(converter.from_bytes(m_name).c_str());
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        return hr;
    }
};
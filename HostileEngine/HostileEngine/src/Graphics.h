#pragma once
#include "GraphicsHelpers.h"
#include "DirectPipeline.h"

#include "Camera.h"

struct alignas(256) ConstantBuffer
{
    Matrix mat;
};

class Graphics// : public IGraphics
{
public:
    enum GRESULT : size_t
    {
        G_OK = 0,
        G_FAIL = 1,
        G_COUNT
    };
    virtual ~Graphics() {}

    GRESULT Init(GLFWwindow* _pWindow);
    GRESULT CreateVertexBuffer(
        std::vector<Vertex> _vertices,
        std::vector<uint32_t> _indices,
        VertexBuffer& _vertexBuffer
    );
    HRESULT CreateTexture(std::string _name, Texture& _texture);

    void BeginFrame();
    void RenderVertexBuffer(
        VertexBuffer& _vertexBuffer, 
        Texture& _texture,
        Matrix _model
    );
    void EndFrame();
    void RenderImGui();

    void OnResize(UINT _width, UINT _height);
    void Update() {}

    void Shutdown();
private:
    HRESULT FindAdapter(ComPtr<IDXGIAdapter>& _adapter);
private:
    HWND m_hwnd;

    ComPtr<ID3D12Device> m_device;
    SwapChain m_swapChain;
    Pipeline m_pipeline;
    ComPtr<ID3D12CommandQueue> m_cmdQueue;
    CommandList m_cmds[FRAME_COUNT];
    CommandList m_loadCmd;
    size_t m_frameIndex;

    bool m_resize = false;
    UINT m_resizeWidth;
    UINT m_resizeHeight;

    ComPtr<ID3D12Resource> m_constBuffers[FRAME_COUNT];
    UINT8* m_cbData[FRAME_COUNT];
    UINT m_constBufferSize = 0;
    UINT m_currentOffset = 0;
    //Matrix m_camera;

    Camera m_camera;
    Vector2 m_currDragDelta;

    ComPtr<ID3D12DescriptorHeap> m_dHeap;
    UINT m_numDescriptors;
    UINT m_srvIncrementSize;
    UINT m_currentDescriptor;

    DirectPipeline m_directPipeline;
};
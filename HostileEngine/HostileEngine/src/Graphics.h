#pragma once
#include "IGraphics.h"
#include "GraphicsHelpers.h"
#include "DirectPipeline.h"

#include "ResourceLoader.h"

#include "Camera.h"

#include <directxtk12/Model.h>
#include <directxtk12/DirectXHelpers.h>
#include <directxtk12/Effects.h>
#include <directxtk12/GraphicsMemory.h>
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/CommonStates.h>
#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/SpriteBatch.h>
#include <directxtk12/DescriptorHeap.h>

namespace Hostile
{
    class RenderContext : public IRenderContext
    {
    public:
        explicit RenderContext(ComPtr<ID3D12Device>& _device);
        virtual ~RenderContext() = default;

        void SetRenderTarget(std::shared_ptr<MoltenRenderTarget>& _rt) final;

        //void SetEffect(
        //    std::shared_ptr<BasicEffect>& _effect
        //);
        //
        //void SetSampler(
        //    D3D12_GPU_DESCRIPTOR_HANDLE _sampler
        //);

        void RenderVertexBuffer(
            MoltenVertexBuffer const& _vertexBuffer,
            Matrix& _world
        ) final;
        void RenderGeometricPrimitive(
            GeometricPrimitive const& _primitive, Matrix const& _world
        ) final;

        void RenderGeometricPrimitive(
            GeometricPrimitive const& _primitive, MoltenTexture const& _texture, Matrix const& _world
        ) final;

        std::shared_ptr<BasicEffect> GetEffect() final
        {
            return m_effect;
        }

        void Wait() final;
        void RenderVertexBuffer(
            MoltenVertexBuffer const& _vb, MoltenTexture const& _mt, std::vector<Matrix> const& _bones, Matrix const& _world
        ) final;

        ComPtr<ID3D12Device>                    m_device;
        std::array<CommandList, FRAME_COUNT>    m_cmds;
        size_t                                  m_currentFrame = 0;

        std::shared_ptr<BasicEffect>            m_effect;
        std::shared_ptr<BasicEffect>            m_currentEffect;
        std::shared_ptr<SkinnedEffect>          m_skinnedEffect;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_sampler;
        D3D12_CPU_DESCRIPTOR_HANDLE             dsv;
    };

    class Graphics : public IGraphics
    {
    public:

        virtual ~Graphics() = default;

        GRESULT Init(GLFWwindow* _pWindow);

        std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            std::unique_ptr<GeometricPrimitive> _primitive
        );
        std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            GeometricPrimitive::VertexCollection& _vertices,
            GeometricPrimitive::IndexCollection& _indices
        );

        std::unique_ptr<MoltenVertexBuffer> CreateVertexBuffer(
            std::vector<VertexPositionNormalTangentColorTextureSkinning>& _vertices,
            std::vector<uint16_t>& _indices
        );

        //Camera& GetSceneCamera();
        //D3D12_GPU_DESCRIPTOR_HANDLE GetRenderTargetSRV(RenderTargets&& _rt);
        //void SetGameCamera(Matrix& _view);
        std::shared_ptr<MoltenRenderTarget> CreateRenderTarget();

        //std::unique_ptr<MoltenTexture> CreateTexture(std::string _name);
        std::shared_ptr<IRenderContext> GetRenderContext()
        {
            return m_renderContexts[0];
        }
        void ExecuteRenderContext(std::shared_ptr<IRenderContext>& _context)
        {
            std::shared_ptr<RenderContext> context = std::dynamic_pointer_cast<RenderContext>(_context);
            std::vector<ID3D12CommandList*> lists = { *context->m_cmds[context->m_currentFrame] };
            context->m_cmds[context->m_currentFrame].cmd->Close();

            m_cmdQueue->ExecuteCommandLists(lists.size(), lists.data());
            context->m_cmds[context->m_currentFrame].m_fenceValue++;
            m_cmdQueue->Signal(context->m_cmds[context->m_currentFrame].m_fence.Get(), context->m_cmds[context->m_currentFrame].m_fenceValue);
        }
        std::unique_ptr<MoltenTexture> CreateTexture(std::string const&& _name);

        void BeginFrame();
        //void RenderIndexed(
        //    MMesh& _mesh,
        //    MTexture& _texture,
        //    std::array<Matrix, MAX_BONES>& _bones
        //);
        void RenderDebug(Matrix& _mat);
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
        ComPtr<IDXGIAdapter3> m_adapter;
        SwapChain m_swapChain;
        Pipeline m_pipeline;
        ComPtr<ID3D12CommandQueue> m_cmdQueue;
        CommandList m_cmds[FRAME_COUNT];
        CommandList m_loadCmd;
        size_t m_frameIndex;

        bool m_resize = false;
        UINT m_resizeWidth;
        UINT m_resizeHeight;

        std::unique_ptr<Model> m_model;
        std::unique_ptr<CommonStates> m_states;
        std::unique_ptr<EffectTextureFactory> m_modelResources;
        std::unique_ptr<EffectFactory> m_fxFactory;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory;

        std::unique_ptr<DescriptorPile> m_resourceDescriptors;
        std::unique_ptr<DescriptorPile> m_renderTargetDescriptors;

        std::vector<std::shared_ptr<RenderContext>> m_renderContexts;

        std::vector<std::shared_ptr<MoltenRenderTarget>> m_renderTargets;
    };
}
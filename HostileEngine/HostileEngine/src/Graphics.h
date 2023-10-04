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
        explicit RenderContext(ComPtr<ID3D12Device> const& _device);
        virtual ~RenderContext() final = default;

        void SetRenderTarget(
            std::shared_ptr<RenderTarget>& _rt,
            std::shared_ptr<DepthTarget>& _dt
        ) final;

        //void SetEffect(
        //    std::shared_ptr<BasicEffect>& _effect
        //);
        //
        //void SetSampler(
        //    D3D12_GPU_DESCRIPTOR_HANDLE _sampler
        //);

        void RenderVertexBuffer(
            VertexBuffer const& _vertexBuffer,
            Matrix& _world
        ) final;
        void RenderGeometricPrimitive(
            GeometricPrimitive const& _primitive, Matrix const& _world
        ) final;

        void RenderGeometricPrimitive(
            GeometricPrimitive const& _primitive, GTexture const& _texture, Matrix const& _world
        ) final;

        std::shared_ptr<BasicEffect> GetEffect() final
        {
            return m_effect;
        }

        std::shared_ptr<BasicEffect> GetStencilEffect() final
        {
            return m_stencilEffect;
        }

        std::shared_ptr<SkinnedEffect> GetSkinnedEffect() final
        {
            return m_skinnedEffect;
        }

        void Wait() final;
        void RenderVertexBuffer(
            VertexBuffer const& _vb, GTexture const& _mt, std::vector<Matrix> const& _bones, Matrix const& _world
        ) final;

        ComPtr<ID3D12Device>                    m_device;
        std::array<CommandList, FRAME_COUNT>    m_cmds;
        size_t                                  m_currentFrame = 0;

        std::shared_ptr<BasicEffect>            m_effect;
        std::shared_ptr<BasicEffect>            m_texturedEffect;
        std::shared_ptr<BasicEffect>            m_stencilEffect;
        std::shared_ptr<BasicEffect>            m_currentEffect;
        std::shared_ptr<SkinnedEffect>          m_skinnedEffect;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_sampler;
        D3D12_CPU_DESCRIPTOR_HANDLE             dsv;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_nullDescriptor;
    };

    class MaterialFactory
    {
    public:
        struct Effect
        {
            ComPtr<ID3D12RootSignature> rootSignature;
            ComPtr<ID3D12PipelineState> pipeline;
        };

        struct Material {};
        explicit MaterialFactory(ComPtr<ID3D12Device>& _device);
        ~MaterialFactory() = default;

        std::shared_ptr<BasicEffect> CreateBasicEffect(
            std::string _name, 
            uint32_t effectFlags, 
            const EffectPipelineStateDescription& pipelineDescription
        );

        enum class Shaders
        {
            VS = 0,
            PS,
            DS,
            HS,
            GS,
            COUNT
        };
        std::shared_ptr<Effect> CreateCustomEffect(
            std::string _name,
            std::array<std::string, static_cast<UINT>(Shaders::COUNT)>& _shaders,
            EffectPipelineStateDescription const& _pipelineDesc
        );

    private:
        ComPtr<ID3D12Device> m_device;

        std::map<std::string, std::shared_ptr<Effect>, std::less<>>                                       m_customEffects;
        std::map<std::string, std::shared_ptr<BasicEffect>, std::less<>>                                    m_basicEffects;
        std::map<std::string, std::array<ComPtr<ID3DBlob>, static_cast<UINT>(Shaders::COUNT)>, std::less<>> m_shaderBlobs;
    };

    class Graphics : public IGraphics
    {
    public:

        ~Graphics() final = default;

        GRESULT Init(GLFWwindow* _pWindow);

        std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            std::unique_ptr<GeometricPrimitive> _primitive
        );
        std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            GeometricPrimitive::VertexCollection& _vertices,
            GeometricPrimitive::IndexCollection& _indices
        );

        std::unique_ptr<VertexBuffer> CreateVertexBuffer(
            std::vector<VertexPositionNormalTangentColorTextureSkinning>& _vertices,
            std::vector<uint16_t>& _indices
        ) final;

        std::shared_ptr<RenderTarget> CreateRenderTarget() final;
        std::shared_ptr<DepthTarget> CreateDepthTarget() final;

        std::shared_ptr<IRenderContext> GetRenderContext() final
        {
            return m_renderContexts[0];
        }
        void ExecuteRenderContext(std::shared_ptr<IRenderContext>& _context) final
        {
            std::shared_ptr<RenderContext> context = std::dynamic_pointer_cast<RenderContext>(_context);
            std::vector<ID3D12CommandList*> lists = { *context->m_cmds[context->m_currentFrame] };
            context->m_cmds[context->m_currentFrame].cmd->Close();

            m_cmdQueue->ExecuteCommandLists(static_cast<UINT>(lists.size()), lists.data());
            context->m_cmds[context->m_currentFrame].m_fenceValue++;
            m_cmdQueue->Signal(context->m_cmds[context->m_currentFrame].m_fence.Get(), context->m_cmds[context->m_currentFrame].m_fenceValue);
        }
        std::unique_ptr<GTexture> CreateTexture(std::string const&& _name) final;

        void BeginFrame() final;
        void RenderDebug(Matrix& _mat) final;
        void EndFrame() final;
        void RenderImGui() final;

        void OnResize(UINT _width, UINT _height) final;
        void Update() final {}

        void Shutdown() final;
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
        UINT m_frameIndex;

        bool m_resize = false;
        UINT m_resizeWidth;
        UINT m_resizeHeight;

        std::unique_ptr<Model> m_model;
        std::unique_ptr<CommonStates> m_states;
        std::unique_ptr<EffectTextureFactory> m_modelResources;
        std::unique_ptr<MaterialFactory> m_fxFactory;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory;

        std::unique_ptr<DescriptorPile> m_resourceDescriptors;
        std::unique_ptr<DescriptorPile> m_renderTargetDescriptors;

        std::vector<std::shared_ptr<RenderContext>> m_renderContexts;

        std::vector<std::shared_ptr<RenderTarget>> m_renderTargets;
        std::vector<std::shared_ptr<DepthTarget>> m_depthTargets;
    };
}
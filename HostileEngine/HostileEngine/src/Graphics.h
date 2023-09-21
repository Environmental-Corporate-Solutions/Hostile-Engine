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
    struct alignas(256) ConstantBuffer
    {
        Matrix mat;
    };

    class Graphics : public IGraphics
    {
    public:
       
        virtual ~Graphics() {}

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

        void RenderGeometricPrimitive(
            std::unique_ptr<GeometricPrimitive>& _primitive,
            Matrix& _world
        );

        void RenderVertexBuffer(
            std::unique_ptr<MoltenVertexBuffer>& vb,
            std::unique_ptr<MoltenTexture>& mt,
            std::vector<Matrix>& bones,
            Matrix& _world
        );

        //std::unique_ptr<MoltenTexture> CreateTexture(std::string _name);

        std::unique_ptr<MoltenTexture> CreateTexture(std::string _name);

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

        ComPtr<ID3D12DescriptorHeap> m_imGuiHeap;

        ComPtr<ID3D12DescriptorHeap> m_dHeap;
        UINT m_numDescriptors;
        UINT m_srvIncrementSize;
        UINT m_currentDescriptor;

        DirectPipeline m_directPipeline;

       std::unique_ptr<Model> m_model;
       std::unique_ptr<CommonStates> m_states;
       std::unique_ptr<EffectTextureFactory> m_modelResources;
       std::unique_ptr<EffectFactory> m_fxFactory;
       //Model::EffectCollection m_modelNormal;
       std::unique_ptr<GraphicsMemory> m_graphicsMemory;
       std::unique_ptr<DescriptorHeap> m_resourceDescriptors;
       std::unique_ptr<DescriptorHeap> m_samplerDescriptors;
       std::unique_ptr<GeometricPrimitive> m_shape;
       std::unique_ptr<BasicEffect> m_effect;
       std::unique_ptr<SpriteBatch> m_spriteBatch;

       std::unique_ptr<SkinnedEffect> m_skinnedEffect;
       
       
       //ModelBone::TransformArray m_drawBones;
       //ModelBone::TransformArray m_animBones;
       //
       //DX::AnimationCMO m_animation;
    };
}
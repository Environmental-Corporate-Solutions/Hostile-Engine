#pragma once
#include "IGraphics.h"
#include "GpuDevice.h"

#include "ResourceLoader.h"
#include "Resources/Pipeline.h"

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

#include "RenderTarget.h"

namespace Hostile
{
    

    class Graphics : public IGraphics
    {
    public:
        Graphics(void) = default;
        ~Graphics() final = default;

        bool Init(GLFWwindow* _pWindow);

        void SetCamera(
            const Vector3& _position,
            const Matrix& _matrix
        ) final;

        void Draw(DrawCall& _draw_call);
        void AddLight(const Light& _light);
        void SetAmbientLight(const Vector4& _ambient);
        std::shared_ptr<IRenderTarget> CreateRenderTarget(UINT _i) final;
        std::shared_ptr<DepthTarget> CreateDepthTarget() final;
        IReadBackBufferPtr CreateReadBackBuffer(
            IRenderTargetPtr& _render_target
        ) final;

        void BeginFrame() final;
        void EndFrame() final;
        void RenderImGui() final;

        void OnResize(UINT _width, UINT _height) final;
        void Update() final { /*not sure this will ever get used lmao*/ }

        void Shutdown() final;
    private:
        void RenderObjects();

    private:
        HWND m_hwnd = nullptr;
        GpuDevice m_device;

        SwapChain                            m_swap_chain{};
        
        std::array<CommandList, g_frame_count> m_cmds{};
        std::array<CommandList, g_frame_count> m_draw_cmds{};
        
        UINT m_frame_index = 0;

        bool m_resize = false;
        UINT m_resize_width = 0;
        UINT m_resize_height = 0;

        std::unique_ptr<CommonStates>   m_states              = nullptr;
        std::unique_ptr<GraphicsMemory> m_graphics_memory      = nullptr;

        enum class GBuffer : UINT
        {
            Color = 0,
            WorldPos = 1,
            Normal = 2,
            Count
        };
        std::array<RenderTargetPtr, static_cast<size_t>(GBuffer::Count)> m_gbuffer{};
        PipelinePtr m_lighting_pipeline;
        std::array<UINT, g_frame_count> m_light_index;
        
        VertexBufferPtr m_frame;

        std::vector<RenderTargetPtr>  m_render_targets{};
        std::vector<std::shared_ptr<DepthTarget>>   m_depth_targets{};

    private:
        Matrix m_camera_matrix;
        Vector3 m_camera_position;
        Vector4 m_ambient_light = { 1, 1, 1, 0.1f };

        std::unordered_map<std::string, PipelinePtr> m_pipelines;

        std::unordered_map<std::string, VertexBufferPtr> m_meshes{};

        std::unordered_map<std::string, MaterialImplPtr> m_materials{};

        std::unordered_map<std::string, TexturePtr> m_textures{};

        std::vector<Light> m_lights{};
    };
}
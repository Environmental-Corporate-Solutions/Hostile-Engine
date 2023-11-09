#pragma once
#include "IGraphics.h"
#include "GpuDevice.h"

#include "ResourceLoader.h"
#include "DirectPipeline.h"

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

        PipelinePtr GetOrLoadPipeline(std::string const& _name);
        VertexBufferPtr GetOrLoadMesh(std::string const& _name);
        MaterialPtr GetOrLoadMaterial(const std::string& _name);
        TexturePtr GetOrLoadTexture(const std::string& _name);

        void SetLight(UINT _light, bool _active);
        void SetLight(UINT _light, const Vector3& _position, const Vector3& _color);

        void Draw(DrawCall& _draw_call);

        VertexBuffer CreateVertexBuffer(
            VertexCollection& _vertices,
            IndexCollection& _indices
        );
        std::shared_ptr<IRenderTarget> CreateRenderTarget(UINT _i) final;
        std::shared_ptr<DepthTarget> CreateDepthTarget() final;
        IReadBackBufferPtr CreateReadBackBuffer(IRenderTargetPtr& _render_target) final;

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

        ComPtr<IDXGIAdapter3>                m_adapter{};
        SwapChain                            m_swapChain{};
        
        std::array<CommandList, g_frame_count> m_cmds{};
        std::array<CommandList, g_frame_count> m_draw_cmds{};
        
        UINT m_frameIndex = 0;

        bool m_resize = false;
        UINT m_resizeWidth = 0;
        UINT m_resizeHeight = 0;

        std::unique_ptr<CommonStates>   m_states              = nullptr;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory      = nullptr;

        std::vector<RenderTargetPtr>  m_render_targets{};
        std::vector<std::shared_ptr<DepthTarget>>   m_depth_targets{};

    private:
        std::unordered_map<std::string, std::shared_ptr<Pipeline>> m_pipelines;

        std::unordered_map<std::string, VertexBufferPtr> m_meshes{};

        std::unordered_map<std::string, MaterialPtr> m_materials{};

        std::unordered_map<std::string, TexturePtr> m_textures{};

        std::array<Light, 16> m_lights{};
    };
}
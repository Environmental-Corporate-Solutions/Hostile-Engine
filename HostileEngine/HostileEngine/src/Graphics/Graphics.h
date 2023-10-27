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
    struct ObjectInstance
    {
        Matrix     world;
        MaterialID material;
        MeshID mesh;
        UINT id;
    };

    struct Light
    {
        XMFLOAT3A lightPosition;
        XMFLOAT4 lightColor;
    };

    struct ShaderConstants
    {
        Matrix viewProjection;
        XMFLOAT3A cameraPosition;
    };

    struct ShaderObject
    {
        Matrix world;
        Matrix normalWorld;
        UINT32 id;
    };

    class Graphics : public IGraphics
    {
    public:
        Graphics(void) = default;
        ~Graphics() final = default;

        bool Init(GLFWwindow* _pWindow);

        void LoadPipeline(std::string const& _name) final;

        MeshID     LoadMesh(std::string const& _name) final;
        MaterialID LoadMaterial(std::string const& _name, std::string const& _pipeline) final;
        MaterialID CreateMaterial(std::string const& _name) final;
        MaterialID CreateMaterial(std::string const& _name, MaterialID const& _id) final;
        InstanceID CreateInstance(MeshID const& _mesh, MaterialID const& _material, UINT32 _id) final;


        LightID    CreateLight() final;
        bool       DestroyLight(LightID const& _light) final;
        bool       UpdateLight(LightID const& _light, Vector3 const& _position, Vector3 const& _color) final;

        bool UpdateInstance(InstanceID const& _instance, Matrix const& _world) final;
        bool UpdateInstance(InstanceID const& _instance, MeshID const& _id) final;
        bool UpdateInstance(InstanceID const& _instance, MaterialID const& _id) final;
        //bool UpdateMaterial(MaterialID const& _id, PBRMaterial const& _material) final;

        void ImGuiMaterialPopup(MaterialID const& _id) final;

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
        std::unique_ptr<Pipeline>            m_pipeline;
        Pipeline::Material m_material;
        ComPtr<ID3D12CommandQueue>           m_cmdQueue{};
        
        std::array<CommandList, g_frame_count> m_cmds{};
        
        UINT m_frameIndex = 0;

        bool m_resize = false;
        UINT m_resizeWidth = 0;
        UINT m_resizeHeight = 0;

        std::unique_ptr<CommonStates>   m_states              = nullptr;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory      = nullptr;

        std::vector<RenderTargetPtr>  m_renderTargets{};
        std::vector<std::shared_ptr<DepthTarget>>   m_depthTargets{};

    private:
        std::unordered_map<std::string, Pipeline> m_pipelines;

        std::map<std::string, MeshID> m_meshIDs{};
        std::map<MeshID, VertexBuffer>             m_meshes{};
        MeshID m_currentMeshID{ 0 };

        std::map<std::string, MaterialID> m_materialIDs{};
        std::map<MaterialID, Pipeline::Material> m_materials{};
        MaterialID m_currentMaterial{ 0 };

        using InstanceList = std::vector<ObjectInstance>;
        using InstanceIDList = std::vector<InstanceID>;
        InstanceList m_objectInstances{};
        InstanceID m_currentInstanceID{ 0 };// = (uint64_t)0;

        std::map<MeshID, InstanceIDList> m_meshInstances{};

        std::array<Light, 16> m_lights{};
    };
}
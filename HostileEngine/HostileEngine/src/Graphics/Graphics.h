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

namespace Hostile
{
    class RenderTarget : public IRenderTarget
    {
    public:
        explicit RenderTarget(ComPtr<ID3D12Device> const& _device, DescriptorPile& _descriptorPile, 
            DXGI_FORMAT _format, Vector2 _dimensions);
        ~RenderTarget() final = default;

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const;
        ComPtr<ID3D12Resource>& GetTexture();

        void IncrementFrameIndex();

        void Clear(CommandList& _cmd);

        D3D12_VIEWPORT GetViewport() const;
        D3D12_RECT GetScissor() const;
    public:
        Vector2 GetDimensions() final;
        UINT64 GetPtr() final;

        void SetView(Matrix const& _view) final;
        void SetCameraPosition(Vector3 const& _cameraPosition) final;
        void SetProjection(Matrix const& _projection) final;
        Matrix GetView() const final;
        Matrix GetProjection() const final;
        Vector3 GetCameraPosition() const final;

    private:
        std::array<ComPtr<ID3D12Resource>, FRAME_COUNT>      m_texture{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FRAME_COUNT> m_rtv{};
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, FRAME_COUNT> m_srv{};
        std::array<UINT64, FRAME_COUNT>                      m_srvIndices{};
        std::unique_ptr<DescriptorHeap>                      m_heap{};

        size_t         m_frameIndex = 0;
        D3D12_VIEWPORT m_vp{};
        D3D12_RECT     m_scissor{};

        D3D12_CLEAR_VALUE m_clearValue{};

        Matrix m_view{};
        Vector3 m_cameraPosition;
        Matrix m_projection{};
    };
    using RenderTargetPtr = std::shared_ptr<RenderTarget>;

    

    struct ObjectInstance
    {
        Matrix     world;
        MaterialID material;
        MeshID mesh;
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
        InstanceID CreateInstance(MeshID const& _mesh, MaterialID const& _material) final;


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
        std::shared_ptr<IRenderTarget> CreateRenderTarget() final;
        std::shared_ptr<DepthTarget> CreateDepthTarget() final;

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
        
        std::array<CommandList, FRAME_COUNT> m_cmds{};
        
        UINT m_frameIndex = 0;

        bool m_resize = false;
        UINT m_resizeWidth = 0;
        UINT m_resizeHeight = 0;

        std::unique_ptr<CommonStates>   m_states              = nullptr;
        std::unique_ptr<GraphicsMemory> m_graphicsMemory      = nullptr;

        std::vector<std::shared_ptr<RenderTarget>>  m_renderTargets{};
        std::vector<std::shared_ptr<DepthTarget>>   m_depthTargets{};

    private:
        std::unordered_map<std::string, Pipeline> m_pipelines;
        
        ComPtr<ID3D12Resource> m_skyboxTexture{};
        size_t m_skyboxTextureIndex = 0;

        ComPtr<ID3D12PipelineState> m_objectPipeline{};
        ComPtr<ID3D12RootSignature> m_objectRootSignature{};

        std::map<std::string, MeshID> m_meshIDs{};
        std::map<MeshID, VertexBuffer>             m_meshes{};
        MeshID m_currentMeshID{ 0 };

        std::map<std::string, MaterialID, std::less<>> m_materialIDs{};
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
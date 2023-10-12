#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <GLFW/glfw3.h>

#include "GraphicsHelpers.h"

#include "Camera.h"

#include <directxtk12/SimpleMath.h>
#include <directxtk12/Effects.h>

#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace Hostile
{
    struct VertexBuffer
    {
        ComPtr<ID3D12Resource>   vb {};
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        ComPtr<ID3D12Resource>   ib {};
        D3D12_INDEX_BUFFER_VIEW  ibv{};
        UINT count = 0;
    };

    struct GTexture
    {
        ComPtr<ID3D12Resource>      tex{};
        D3D12_GPU_DESCRIPTOR_HANDLE srv{};
    };

    class IRenderTarget
    {
    public:
        virtual ~IRenderTarget() = default;

        virtual void SetView(Matrix const& _view) = 0;
        virtual void SetProjection(Matrix const& _projection) = 0;
        virtual void SetCameraPosition(Vector3 const& _cameraPosition) = 0;
        virtual Matrix GetView() const = 0;
        virtual Matrix GetProjection() const = 0;
        virtual Vector3 GetCameraPosition() const = 0;

        virtual Vector2 GetDimensions() = 0;
        virtual UINT64 GetPtr() = 0;
    };
    using IRenderTargetPtr = std::shared_ptr<IRenderTarget>;


    struct DepthTarget
    {
        std::array<ComPtr<ID3D12Resource>, FRAME_COUNT> textures{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FRAME_COUNT> dsvs{};
        std::unique_ptr<DescriptorHeap> heap{};
        UINT frameIndex = 0;
    };

    static constexpr uint64_t INVALID_ID = uint64_t(-1);
    using PipelineID = uint64_t;
    using MeshID     = uint64_t;
    using InstanceID = uint64_t;
    using MaterialID = uint64_t;

    struct PBRMaterial
    {
        Vector3 albedo = { 1, 1, 1 };
        float metalness = 0.5f;
        float roughness = 0.5f;
    };

    class IGraphics
    {
    public:
        virtual ~IGraphics() = default;

        virtual bool Init(GLFWwindow* _pWindow) = 0;

        virtual MeshID     LoadMesh(std::string const& _name) = 0;
        virtual MaterialID LoadMaterial(std::string const& _name) = 0;
        virtual MaterialID CreateMaterial(std::string const& _name) = 0;
        virtual MaterialID CreateMaterial(std::string const& _name, MaterialID const& _id) = 0;
        virtual InstanceID CreateInstance(MeshID const& _mesh, MaterialID const& _material) = 0;

        virtual bool UpdateInstance(InstanceID const& _instance, Matrix const& _world) = 0;
        virtual bool UpdateMaterial(MaterialID const& _id, PBRMaterial const& _material) = 0;

        const size_t MAX_RENDER_TARGETS = 4;

        virtual std::shared_ptr<IRenderTarget> CreateRenderTarget() = 0;
        virtual std::shared_ptr<DepthTarget> CreateDepthTarget() = 0;

        virtual void BeginFrame() = 0;
        
        virtual void EndFrame() = 0;
        virtual void RenderImGui() = 0;

        virtual void OnResize(UINT _width, UINT _height) = 0;;
        virtual void Update() = 0;

        virtual void Shutdown() = 0;

        static IGraphics& Get();
    };
}
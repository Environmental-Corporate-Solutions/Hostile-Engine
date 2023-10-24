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
    ComPtr<ID3D12Resource>   vb{};
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    ComPtr<ID3D12Resource>   ib{};
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
    struct GraphicsID
    {
        GraphicsID(uint64_t _other) : m_id(_other) {}
        GraphicsID(void) {};
        uint64_t m_id = 0;
        virtual explicit operator uint64_t& ()
        {
            return m_id;
        }

        virtual explicit operator uint64_t const& () const
        {
            return m_id;
        }

        virtual GraphicsID& operator=(uint64_t _other)
        {
            m_id = _other;
            return *this;
        }

        virtual GraphicsID& operator++(int)
        {
            m_id++;
            return *this;
        }

        bool operator==(uint64_t _id) const
        {
            return m_id == _id;
        }
        bool operator!=(uint64_t _id) const
        {
            return m_id != _id;
        }
        bool operator<(uint64_t _id) const
        {
            return m_id < _id;
        }

        bool operator==(GraphicsID _id) const
        {
            return m_id == _id.m_id;
        }
        bool operator!=(GraphicsID _id) const
        {
            return m_id != _id.m_id;
        }
        bool operator<(GraphicsID _id) const
        {
            return m_id < _id.m_id;
        }
    };
#define DECLARE_ID(x)               \
    struct x : public GraphicsID {  \
        using GraphicsID::GraphicsID;\
        using GraphicsID::operator=;\
        using GraphicsID::operator++;\
        using GraphicsID::operator uint64_t&;\
        using GraphicsID::operator==;\
        virtual ~x() {}}


    DECLARE_ID(PipelineID);
    DECLARE_ID(MeshID);
    DECLARE_ID(InstanceID);
    DECLARE_ID(MaterialID);
    DECLARE_ID(LightID);

    struct PBRMaterial
    {
        Vector3 albedo = { 1, 1, 1 };
        float metalness = 0.5f;
        float roughness = 0.5f;
        float emissive = 0.0f;
    };


    class IGraphics
    {
    public:
        virtual ~IGraphics() = default;

    virtual bool Init(GLFWwindow* _pWindow) = 0;

        virtual void LoadPipeline(std::string const& _name) = 0;

        virtual void LoadPipeline(std::string const& _name) = 0;

        virtual MeshID     LoadMesh(std::string const& _name) = 0;
        virtual MaterialID LoadMaterial(std::string const& _name, std::string const& _pipeline) = 0;
        virtual MaterialID CreateMaterial(std::string const& _name) = 0;
        virtual MaterialID CreateMaterial(std::string const& _name, MaterialID const& _id) = 0;
        virtual InstanceID CreateInstance(MeshID const& _mesh, MaterialID const& _material) = 0;
        
        virtual LightID    CreateLight() = 0;
        virtual bool       DestroyLight(LightID const& _light) = 0;
        virtual bool       UpdateLight(LightID const& _light, Vector3 const& _position, Vector3 const& _color) = 0;

        virtual bool UpdateInstance(InstanceID const& _instance, Matrix const& _world) = 0;
        virtual bool UpdateInstance(InstanceID const& _instance, MeshID const& _id) = 0;
        virtual bool UpdateInstance(InstanceID const& _instance, MaterialID const& _id) = 0;
        //virtual bool UpdateMaterial(MaterialID const& _id, PBRMaterial const& _material) = 0;
        virtual void ImGuiMaterialPopup(MaterialID const& _id) = 0;

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
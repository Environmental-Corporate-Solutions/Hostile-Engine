#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <GLFW/glfw3.h>

#include <directxtk12/GeometricPrimitive.h>
#include <directxtk12/SimpleMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace Hostile
{
    struct VertexPositionNormalTangentColorTextureSkinning
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 tangent;
        uint32_t color;
        XMFLOAT2 textureCoordinate;
        uint32_t indices;
        uint32_t weights;

        void SetBlendIndices(XMUINT4 const& iindices) noexcept
        {
            this->indices = ((iindices.w & 0xff) << 24) | ((iindices.z & 0xff) << 16) | ((iindices.y & 0xff) << 8) | (iindices.x & 0xff);
        }

        void SetBlendWeights(XMFLOAT4 const& iweights) noexcept { SetBlendWeights(XMLoadFloat4(&iweights)); }
        void XM_CALLCONV SetBlendWeights(FXMVECTOR iweights) noexcept
        {
            using namespace DirectX::PackedVector;

            XMUBYTEN4 packed;
            XMStoreUByteN4(&packed, iweights);
            this->weights = packed.v;
        }

        static const D3D12_INPUT_LAYOUT_DESC InputLayout;

    private:
        static constexpr unsigned int InputElementCount = 7;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    struct MoltenVertexBuffer
    {
        ComPtr<ID3D12Resource> vb;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        ComPtr<ID3D12Resource> ib;
        D3D12_INDEX_BUFFER_VIEW ibv;
        size_t count;
    };

    struct MoltenTexture
    {
        ComPtr<ID3D12Resource> tex;
        uint32_t index;
    };

    class IGraphics
    {
    public:
        virtual ~IGraphics() {}

        enum GRESULT : size_t
        {
            G_OK = 0,
            G_FAIL = 1,
            G_COUNT
        };

        virtual GRESULT Init(GLFWwindow* _pWindow) = 0;

        virtual std::unique_ptr<MoltenVertexBuffer> CreateVertexBuffer(
            std::vector<VertexPositionNormalTangentColorTextureSkinning>& _vertices,
            std::vector<uint16_t>& _indices
        ) = 0;

        virtual std::unique_ptr<MoltenTexture> CreateTexture(std::string _name) = 0;

        virtual std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            std::unique_ptr<GeometricPrimitive> _primitive
        ) = 0;
        virtual std::unique_ptr<GeometricPrimitive> CreateGeometricPrimitive(
            GeometricPrimitive::VertexCollection& _vertices,
            GeometricPrimitive::IndexCollection& _indices
        ) = 0;

        virtual void RenderGeometricPrimitive(
            std::unique_ptr<GeometricPrimitive>& _primitive,
            Matrix& _world
        ) = 0;

        virtual void RenderVertexBuffer(
            std::unique_ptr<MoltenVertexBuffer>& vb,
            std::unique_ptr<MoltenTexture>& mt,
            std::vector<Matrix>& bones,
            Matrix& _world
        ) = 0;

        virtual void BeginFrame() = 0;
        

        virtual void RenderDebug(Matrix& _mat) = 0;
        virtual void EndFrame() = 0;
        virtual void RenderImGui() = 0;

        virtual void OnResize(UINT _width, UINT _height) = 0;;
        virtual void Update() = 0;

        virtual void Shutdown() = 0;

        static IGraphics& Get();
    };
}
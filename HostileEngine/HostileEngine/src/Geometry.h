//--------------------------------------------------------------------------------------
// File: Geometry.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once
#include <vector>
#include <directxtk12/SimpleMath.h>
#include <d3d12.h>
#include <array>

namespace Hostile
{
    using namespace DirectX::SimpleMath;
    using namespace DirectX;
    struct SkinnedVertex
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
        static const std::array<D3D12_INPUT_ELEMENT_DESC, InputElementCount> InputElements;
    };

    struct PrimitiveVertex
    {
        PrimitiveVertex(
            DirectX::SimpleMath::Vector3 const& _position,
            DirectX::SimpleMath::Vector3 const& _normal,
            DirectX::SimpleMath::Vector2 const& _textureCoordinate
        ) : position(_position), normal(_normal), textureCoordinate(_textureCoordinate)
        {
        }

        PrimitiveVertex(
            DirectX::XMVECTOR const& _position,
            DirectX::XMVECTOR const& _normal,
            DirectX::XMVECTOR const& _textureCoordinate
        ) : position(_position), normal(_normal), textureCoordinate(_textureCoordinate)
        {
        }
        DirectX::SimpleMath::Vector3 position;
        DirectX::SimpleMath::Vector3 normal;
        DirectX::SimpleMath::Vector2 textureCoordinate;

    private:
        static constexpr unsigned int InputElementCount = 3;
        static constexpr std::array<D3D12_INPUT_ELEMENT_DESC, InputElementCount> InputElements =
        {
            D3D12_INPUT_ELEMENT_DESC{ "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

    public:
        static constexpr D3D12_INPUT_LAYOUT_DESC InputLayout = { InputElements.data(), (UINT)InputElements.size() };
    };

    using VertexCollection = std::vector<PrimitiveVertex>;
    using IndexCollection = std::vector<uint16_t>;

    void ComputeBox(VertexCollection& vertices, IndexCollection& indices, const XMFLOAT3& size, bool rhcoords, bool invertn);
    void ComputeSphere(VertexCollection& vertices, IndexCollection& indices, float diameter, size_t tessellation, bool rhcoords, bool invertn);
    void ComputeGeoSphere(VertexCollection& vertices, IndexCollection& indices, float diameter, size_t tessellation, bool rhcoords);
    void ComputeCylinder(VertexCollection& vertices, IndexCollection& indices, float height, float diameter, size_t tessellation, bool rhcoords);
    void ComputeCone(VertexCollection& vertices, IndexCollection& indices, float diameter, float height, size_t tessellation, bool rhcoords);
    void ComputeTorus(VertexCollection& vertices, IndexCollection& indices, float diameter, float thickness, size_t tessellation, bool rhcoords);
    void ComputeTetrahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeOctahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeDodecahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
    void ComputeIcosahedron(VertexCollection& vertices, IndexCollection& indices, float size, bool rhcoords);
}
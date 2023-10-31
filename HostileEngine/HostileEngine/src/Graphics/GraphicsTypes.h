#pragma once
#include <wrl.h>
#include <array>
#include <memory>
#include <d3d12.h>
#include "GraphicsHelpers.h"
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/GraphicsMemory.h>
#include <variant>

namespace Hostile
{
    using D3D12ResourcePtr = Microsoft::WRL::ComPtr<ID3D12Resource>;
    struct VertexBuffer
    {
        std::string name;
        D3D12ResourcePtr   vb{};
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        D3D12ResourcePtr   ib{};
        D3D12_INDEX_BUFFER_VIEW  ibv{};
        UINT count = 0;
    };
    using VertexBufferPtr = std::shared_ptr<VertexBuffer>;

    struct Texture
    {
        std::string name;
        ComPtr<ID3D12Resource> texture;
        UINT index;
        D3D12_GPU_DESCRIPTOR_HANDLE handle;
    };
    using TexturePtr = std::shared_ptr<Texture>;

    struct DepthTarget
    {
        std::array<D3D12ResourcePtr, g_frame_count> textures{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, g_frame_count> dsvs{};
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, g_frame_count> srvs{};
        std::unique_ptr<DirectX::DescriptorHeap> heap{};
        UINT frameIndex = 0;
    };

    struct Light
    {
        DirectX::XMFLOAT3A lightPosition;
        DirectX::XMFLOAT4 lightColor;
    };

    struct ShaderConstants
    {
        Matrix viewProjection;
        DirectX::XMFLOAT3A cameraPosition;
    };

    struct ShaderObject
    {
        DirectX::SimpleMath::Matrix world;
        DirectX::SimpleMath::Matrix normalWorld;
        UINT32 id;
    };

    struct MaterialInput
    {
        enum class Type
        {
            TEXTURE = 0,
            FLOAT,
            FLOAT2,
            FLOAT3,
            FLOAT4,
            INVALID
        };
        static Type TypeFromString(std::string const& _str);
        static constexpr std::array typeSizes = {
            sizeof(float), sizeof(Vector2), sizeof(Vector3),
            sizeof(Vector4)
        };
        std::string name;
        Type type;
        std::variant<Texture, float, Vector2, Vector3, Vector4> value;
    };

    class Pipeline;
    using PipelinePtr = std::shared_ptr<Pipeline>;
    struct Material
    {
        std::string name;
        std::vector<MaterialInput> m_material_inputs;
        size_t m_size;

        PipelinePtr m_pipeline = nullptr;
        DirectX::GraphicsResource m_resource;

        void SetPipeline(PipelinePtr _pipeline);
        void UpdateValues();
    };
    using MaterialPtr = std::shared_ptr<Material>;

    struct InstanceData
    {
        MaterialPtr m_material;
        VertexBufferPtr m_vertex_buffer;
        UINT32 m_id;
        UINT32 m_stencil = 0;
    };

    struct DrawCall
    {
        DirectX::SimpleMath::Matrix world;
        InstanceData instance;
    };
}
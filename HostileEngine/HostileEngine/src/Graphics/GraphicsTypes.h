#pragma once
#include <wrl.h>
#include <array>
#include <memory>
#include <d3d12.h>
#include "GraphicsHelpers.h"
#include <directxtk12/DescriptorHeap.h>
#include <directxtk12/GraphicsMemory.h>
#include <variant>

#include "Resources/VertexBuffer.h"
#include "Resources/Texture.h"

namespace Hostile
{
    using D3D12ResourcePtr = Microsoft::WRL::ComPtr<ID3D12Resource>;

    struct DepthTarget
    {
        std::array<D3D12ResourcePtr, g_frame_count> textures{};
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, g_frame_count> dsvs{};
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, g_frame_count> srvs{};
        std::unique_ptr<DirectX::DescriptorHeap> heap{};
        UINT frameIndex = 0;
    };

    struct alignas(256) Light
    {
        DirectX::XMFLOAT3 lightPosition;
        DirectX::XMFLOAT3 lightColor;
    };

    struct alignas(256) ShaderConstants
    {
        Matrix view_projection;
        DirectX::XMFLOAT3A camera_position;
        DirectX::XMFLOAT4 ambient_light;
    };

    struct alignas(256) ShaderObject
    {
        DirectX::SimpleMath::Matrix world;
        DirectX::SimpleMath::Matrix normalWorld;
        UINT32 id;
    };

    struct MaterialInput
    {
        enum class Type
        {
            Texture = 0,
            Buffer
        };
        std::string name;
        std::variant<std::string, float, Vector2, Vector3, Vector4> value;
    };
    using MaterialInputMap = std::unordered_map<std::string, MaterialInput>;

    struct MaterialTexture
    {
        std::string name;
        UINT bind_point;
        std::string scratch = "";
    };

    class MaterialImpl;

    using MaterialImplPtr = std::shared_ptr<MaterialImpl>;
    struct Material
    {
        MaterialImplPtr materal;
    };
    struct Renderer
    {
        MaterialImplPtr m_material;
        VertexBufferPtr m_vertex_buffer;
        UINT32 m_id;
        UINT32 m_stencil = 0;
    };

    struct DrawCall
    {
        DirectX::SimpleMath::Matrix world;
        Renderer instance;
    };
}
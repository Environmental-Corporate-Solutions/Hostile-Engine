#pragma once
#include <array>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <array>
#include <directxtk12/SimpleMath.h>
#include <directxtk12/DirectXHelpers.h>
#include <DirectXMath.h>
#include <wrl.h>

#define MAX_BONES 200
#define MAX_WEIGHTS 8

namespace Hostile
{
    using namespace Microsoft::WRL;
    using namespace DirectX::SimpleMath;
    using namespace DirectX;

    struct MeshData
    {
        struct Entry
        {
            std::string name;
            size_t baseVertex;
        };

        struct VertexBone
        {
            Vector3 pos;
            Vector3 normal;
            Vector2 texCoord;
            Vector4 weights;
            XMUINT4 m_bones;

            uint32_t currentBone = 0;
            void AddBone(uint32_t _bone, float _weight)
            {
                switch (currentBone)
                {
                case 0:
                    weights.x = _weight;
                    m_bones.x = _bone;
                    break;

                case 1:
                    weights.y = _weight;
                    m_bones.y = _bone;
                    break;

                case 2:
                    weights.z = _weight;
                    m_bones.z = _bone;
                    break;

                case 3:
                    weights.w = _weight;
                    m_bones.w = _bone;
                    break;
                }
                currentBone++;
            }
        };
        std::vector<VertexBone> vertices;
        std::vector<uint16_t> faces;
        std::vector<Entry> entries;
    };

    struct AnimationNode
    {
        struct VectorKey
        {
            double time;
            Vector3 value;
        };

        struct QuatKey
        {
            double time;
            Quaternion value;
        };

        std::string nodeName;
        size_t nodeIndex;
        std::vector<VectorKey> positionKeys;
        std::vector<QuatKey> rotationKeys;
        std::vector<VectorKey> scalingKeys;
    };

    struct Animation
    {
        std::string name = "";
        float duration = 0;
        float ticksPerSec = 0;
        float timeInSeconds = 0;
        float numChannels = 0;
        std::vector<AnimationNode> nodes;
    };

    struct Skeleton
    {
        struct Entry
        {
            std::string name;
            size_t baseVertex;
        };

        struct Node
        {
            std::string name;
            Matrix transformation;
            std::vector<Node> children;
        };

        struct Bone
        {
            std::string name;
            Matrix offset;
            Matrix finalTransform;
        };

        std::vector<Bone> bones;
        std::map<std::string, size_t> boneMapping;
        std::vector<Entry> entries;
        Matrix globalInverseTransform;
        Node rootNode;
    };

    struct SceneData
    {
        MeshData meshData;
        Skeleton skeleton;
        std::vector<Animation> animations;
    };

    struct VertexBuffer
    {
        ComPtr<ID3D12Resource> vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vbView;
    };

    struct IndexBuffer
    {
        ComPtr<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW ibView;
        size_t count;
    };

    struct MTexture
    {
        ComPtr<ID3D12Resource> texture;
    };

    struct MMesh
    {
        std::array<VertexBuffer, 5> vertexBuffers;
        IndexBuffer indexBuffer;
    };

    SceneData LoadSceneFromFile(std::string&& _filepath);

}
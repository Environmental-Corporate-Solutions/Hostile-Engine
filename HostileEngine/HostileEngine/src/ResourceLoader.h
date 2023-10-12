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

#include "Geometry.h"

#define MAX_BONES 200
#define MAX_WEIGHTS 8

namespace Hostile
{
    using namespace Microsoft::WRL;
    using namespace DirectX::SimpleMath;
    using namespace DirectX;

    

    struct AnimationNode
    {
        struct VectorKey
        {
            float time;
            Vector3 value;
        };

        struct QuatKey
        {
            float time;
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
        std::vector<Matrix> boneMatrices;
        std::vector<UINT> joints{};
        std::vector<Matrix> inverseBindMatrices{};
        UINT skeleton = 0;
        Matrix globalInverseTransform;
    };

    struct MeshData
    {
        struct Entry
        {
            std::string name;
            size_t baseVertex;
        };
        std::vector<Entry> entries{};
        std::vector<SkinnedVertex> vertices{};
        std::vector<uint16_t> indices{};
    };

    struct Node
    {
        std::string name = "";
        UINT        camera = UINT(-1);
        UINT        mesh = UINT(-1);
        UINT        skin = UINT(-1);

        Vector3     translation{};
        Quaternion  rotation = Quaternion::Identity;
        Vector3     scale = Vector3::One;
        Matrix      matrix = Matrix::Identity;

        std::vector<UINT> children{};
    };

    struct SceneData
    {
        std::string name;
        std::vector<Node> nodes;
        MeshData meshData;
        Skeleton skeleton;
        std::vector<Animation> animations;
    };

    struct MTexture
    {
        ComPtr<ID3D12Resource> texture;
    };

    SceneData LoadSceneFromFile(std::string& _filepath);
    SceneData LoadAnimationFromFile(std::string& _filepath);
}
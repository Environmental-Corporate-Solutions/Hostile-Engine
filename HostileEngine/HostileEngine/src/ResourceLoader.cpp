#include <stdafx.h>
#include "ResourceLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <array>
#include <map>
#include <memory>


#include <fbxsdk.h>

#include <nlohmann/json.hpp>
#include <fstream>

namespace Hostile
{
    enum class BufferViewType
    {
        VERTEX_BUFFER = 34962,
        INDEX_BUFFER = 34963
    };

    enum class AccessorComponentType
    {
        BYTE = 5120,
        UNSIGNED_BYTE = 5121,
        SHORT = 5122,
        UNSIGNED_SHORT = 5123,
        UNSIGNED_INT = 5125,
        FLOAT = 5126
    };

    struct Scene
    {
        std::string name;
        std::vector<UINT> nodes;
    };

    struct GLTFMeshPrimitiveAttribute
    {
        std::string name     = "";
        UINT        accessor = -1;
    };

    struct GLTFMeshPrimitive
    {
        UINT material = -1;
        UINT mode     = -1;
        UINT indices  = -1;
        std::vector<GLTFMeshPrimitiveAttribute> attributes;
    };

    struct GLTFMesh
    {
        std::string name;
        std::vector<GLTFMeshPrimitive> primitives;
    };

    struct Buffer
    {
        std::vector<UINT8> data;
        UINT byteLength;
    };

    struct BufferView
    {
        std::string name = "";
        UINT buffer      = 0;
        UINT byteOffset  = 0;
        UINT byteLength  = -1;
        UINT byteStride  = -1;
        UINT target      = 0;
    };

    struct Accessor
    {
        std::string name          = "";
        UINT        bufferView    = -1;
        UINT        byteOffset    = -1;
        UINT        componentType = -1;
        bool        normalize     = false;
        UINT        count         = 0;
        std::string type          = "";

        Matrix min;
        Matrix max;
    };

    struct Skin
    {
        std::vector<UINT> joints{};
        std::vector<Matrix> inverseBindMats{};
        UINT invertedBoneMatrices = 0;
        UINT skeleton             = 0;
    };

    SceneData LoadSceneFromFile(std::string& _filepath)
    {
        using namespace nlohmann;

        std::ifstream f(_filepath);
        json data;
        try
        {
            data = json::parse(f);
        }
        catch (detail::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        std::vector<Node>       nodes;
        std::vector<GLTFMesh>   meshes;
        std::vector<Buffer>     buffers;
        std::vector<BufferView> bufferViews;
        std::vector<Accessor>   accessors;
        std::vector<Skin>       skins;

        for (auto const& buffer : data["buffers"])
        {
            Buffer b{};
            b.byteLength = buffer["byteLength"].get<UINT>();
            b.data.resize(b.byteLength);

            std::ifstream bufferFile("Assets/models/Bear_out/" + buffer["uri"].get<std::string>(), std::ios::binary);
            std::streamsize total = 0;
            while (total < b.byteLength)
            {
                bufferFile.read(reinterpret_cast<char*>(b.data.data()), b.byteLength - total);
                total += bufferFile.gcount();
            }

            buffers.push_back(b);
        }

        for (auto const& it : data["scenes"])
        {
            Scene scene;
            scene.name = it["name"].get<std::string>();
            for (auto const& node : it["nodes"])
            {
                scene.nodes.push_back(node.get<UINT>());
            }
        }

        for (auto const& node : data["nodes"])
        {
            Node n{};
            n.name = node["name"];

            if (node.contains("translation"))
                n.translation = { node["translation"][0].get<float>(), node["translation"][1].get<float>(), node["translation"][2].get<float>() };

            if (node.contains("rotation"))
                n.rotation = { node["rotation"][0].get<float>(), node["rotation"][1].get<float>(), node["rotation"][2].get<float>(), node["rotation"][3].get<float>() };

            if (node.contains("scale"))
                n.scale = { node["scale"][0].get<float>(), node["scale"][1].get<float>(), node["scale"][2].get<float>() };

            if (node.contains("matrix"))
            {
                for (int i = 0; i < 12; i++)
                {
                    n.matrix.m[i % 4][i / 4] = node["matrix"][i].get<float>();
                }
            }

            if (node.contains("camera"))
                n.camera = node["camera"].get<UINT>();

            if (node.contains("mesh"))
                n.mesh = node["mesh"].get<UINT>();

            if (node.contains("skin"))
                n.skin = node["skin"].get<UINT>();

            if (node.contains("children"))
            {
                for (auto const& child : node["children"])
                {
                    n.children.push_back(child.get<UINT>());
                }
            }
            nodes.push_back(n);
        }

        for (auto const& bview : data["bufferViews"])
        {
            BufferView b{};

            if (bview.contains("name"))
                b.name = bview["name"].get<std::string>();

            if (bview.contains("byteOffset"))
                b.byteOffset = bview["byteOffset"].get<UINT>();

            if (bview.contains("byteLength"))
                b.byteLength = bview["byteLength"].get<UINT>();

            if (bview.contains("byteStride"))
                b.byteStride = bview["byteStride"].get<UINT>();

            if (bview.contains("target"))
                b.target = bview["target"].get<UINT>();

            bufferViews.push_back(b);
        }

        for (auto const& accessor : data["accessors"])
        {
            Accessor a{};

            if (accessor.contains("name"))
                a.name = accessor["name"].get<std::string>();

            if (accessor.contains("bufferView"))
                a.bufferView = accessor["bufferView"].get<UINT>();

            if (accessor.contains("byteOffset"))
                a.byteOffset = accessor["byteOffset"].get<UINT>();

            a.componentType = accessor["componentType"].get<UINT>();

            if (accessor.contains("normalized"))
                a.normalize = accessor["normalized"].get<bool>();

            a.count = accessor["count"].get<UINT>();
            a.type = accessor["type"].get<std::string>();

            if (accessor.contains("max"))
            {
                if (a.type == "SCALAR")
                {
                    a.max.m[0][0] = accessor["max"].get<float>();
                }
                else if (a.type == "VEC2")
                {
                    a.max.m[0][0] = accessor["max"][0].get<float>();
                    a.max.m[0][1] = accessor["max"][1].get<float>();
                }
                else if (a.type == "VEC3")
                {
                    a.max.m[0][0] = accessor["max"][0].get<float>();
                    a.max.m[0][1] = accessor["max"][1].get<float>();
                    a.max.m[0][2] = accessor["max"][2].get<float>();
                }
                else if (a.type == "VEC4")
                {
                    a.max.m[0][0] = accessor["max"][0].get<float>();
                    a.max.m[0][1] = accessor["max"][1].get<float>();
                    a.max.m[0][2] = accessor["max"][2].get<float>();
                    a.max.m[0][3] = accessor["max"][3].get<float>();
                }
                else if (a.type == "MAT4")
                {
                    for (int i = 0; i < 12; i++)
                    {
                        a.max.m[i % 4][i / 4] = accessor["max"][i].get<float>();
                    }
                }
            }

            if (accessor.contains("min"))
            {
                if (a.type == "SCALAR")
                {
                    a.min.m[0][0] = accessor["min"].get<float>();
                }
                else if (a.type == "VEC2")
                {
                    a.min.m[0][0] = accessor["min"][0].get<float>();
                    a.min.m[0][1] = accessor["min"][1].get<float>();
                }
                else if (a.type == "VEC3")
                {
                    a.min.m[0][0] = accessor["min"][0].get<float>();
                    a.min.m[0][1] = accessor["min"][1].get<float>();
                    a.min.m[0][2] = accessor["min"][2].get<float>();
                }
                else if (a.type == "VEC4")
                {
                    a.min.m[0][0] = accessor["min"][0].get<float>();
                    a.min.m[0][1] = accessor["min"][1].get<float>();
                    a.min.m[0][2] = accessor["min"][2].get<float>();
                    a.min.m[0][3] = accessor["min"][3].get<float>();
                }
                else if (a.type == "MAT4")
                {
                    for (int i = 0; i < 12; i++)
                    {
                        a.min.m[i % 4][i / 4] = accessor["min"][i].get<float>();
                    }
                }
            }

            accessors.push_back(a);
        }

        for (auto const& skin : data["skins"])
        {
            Skin s{};
            for (auto const& joint : skin["joints"])
            {
                s.joints.push_back(joint.get<UINT>());
            }
            if (skin.contains("inverseBindMatrices"))
                s.invertedBoneMatrices = skin["inverseBindMatrices"].get<UINT>();
            if (skin.contains("skeleton"))
                s.skeleton = skin["skeleton"].get<UINT>();

            skins.push_back(s);
        }

        SceneData sd{};
        for (auto const& mesh : data["meshes"])
        {
            sd.meshData.entries.push_back({ "", sd.meshData.vertices.size() });
            for (auto const& primitive : mesh["primitives"])
            {
                {
                    Accessor& a = accessors[primitive["attributes"]["POSITION"].get<UINT>()];
                    BufferView& b = bufferViews[a.bufferView];
                    Buffer& buffer = buffers[b.buffer];

                    for (UINT i = 0; i < a.count; i++)
                    {
                        VertexPositionNormalTangentColorTextureSkinning v;
                        UINT index = (i * sizeof(Vector3)) + b.byteOffset + a.byteOffset;
                        memcpy(&v.position, &buffer.data[index], sizeof(Vector3));
                        sd.meshData.vertices.push_back(v);
                    }
                }

                {
                    Accessor& a = accessors[primitive["attributes"]["NORMAL"].get<UINT>()];
                    BufferView& b = bufferViews[a.bufferView];
                    Buffer& buffer = buffers[b.buffer];

                    for (UINT i = 0; i < a.count; i++)
                    {
                        UINT index = (i * sizeof(Vector3)) + b.byteOffset + a.byteOffset;
                        memcpy(&sd.meshData.vertices[i].normal, &buffer.data[index], sizeof(Vector3));
                    }
                }

                {
                    Accessor& a = accessors[primitive["attributes"]["TEXCOORD_0"].get<UINT>()];
                    BufferView& b = bufferViews[a.bufferView];
                    Buffer& buffer = buffers[b.buffer];

                    for (UINT i = 0; i < a.count; i++)
                    {
                        UINT index = (i * sizeof(Vector2)) + b.byteOffset + a.byteOffset;
                        memcpy(&sd.meshData.vertices[i].textureCoordinate, &buffer.data[index], sizeof(Vector2));
                    }
                }

                {
                    Accessor& a    = accessors[primitive["attributes"]["JOINTS_0"].get<UINT>()];
                    Accessor& a2   = accessors[primitive["attributes"]["WEIGHTS_0"].get<UINT>()];
                    BufferView& b  = bufferViews[a.bufferView];
                    BufferView& b2 = bufferViews[a2.bufferView];
                    Buffer& buffer = buffers[b.buffer];

                    for (UINT i = 0; i < a.count; i++)
                    {
                        UINT index = (i * (sizeof(USHORT) * 4)) + b.byteOffset + a.byteOffset;
                        std::array<USHORT, 4> joints;
                        memcpy(joints.data(), &buffer.data[index], (sizeof(USHORT) * 4));
                        sd.meshData.vertices[i].SetBlendIndices({ joints[0], joints[1], joints[2], joints[3] });

                        XMFLOAT4 weights;
                        index = (i * sizeof(XMFLOAT4)) + b2.byteOffset + a2.byteOffset;
                        memcpy(&weights, &buffer.data[index], sizeof(XMFLOAT4));
                        sd.meshData.vertices[i].SetBlendWeights(weights);
                    }
                }

                {
                    Accessor& a = accessors[primitive["indices"].get<UINT>()];
                    BufferView& b = bufferViews[a.bufferView];
                    Buffer& buffer = buffers[b.buffer];

                    size_t off = sd.meshData.indices.size();
                    sd.meshData.indices.resize(a.count + sd.meshData.indices.size());
                    
                    for (UINT i = 0; i < a.count; i++)
                    {
                        UINT index = (i * sizeof(USHORT)) + b.byteOffset + a.byteOffset;
                        memcpy(&sd.meshData.indices[i + off], &buffer.data[index], sizeof(USHORT));
                    }
                }
            }
        }

        sd.skeleton.joints = skins[0].joints;
        sd.skeleton.skeleton = skins[0].skeleton;
        {
            Accessor&   a      = accessors[skins[0].invertedBoneMatrices];
            BufferView& b      = bufferViews[a.bufferView];
            Buffer&     buffer = buffers[b.buffer];
            sd.skeleton.inverseBindMatrices.resize(sd.skeleton.joints.size());
            sd.skeleton.boneMatrices.resize(sd.skeleton.joints.size());

            for (int i = 0; i < a.count; i++)
            {
                UINT index = (i * sizeof(Matrix)) + b.byteOffset + a.byteOffset;
                memcpy(&sd.skeleton.inverseBindMatrices[i], &buffer.data[index], sizeof(Matrix));
            }
        }
        sd.nodes = nodes;

        return sd;
    }

    SceneData LoadAnimationFromFile(std::string& _filepath)
    {
        using namespace nlohmann;
        SceneData sd{};
        try
        {
            std::ifstream f(_filepath);
            json data = json::parse(f);

            std::vector<Buffer> buffers;

            for (auto const& buffer : data["buffers"])
            {
                Buffer b{};
                b.byteLength = buffer["byteLength"].get<UINT>();
                b.data.resize(b.byteLength);

                std::ifstream bufferFile("Assets/models/Bear_WalkForward_out/" + buffer["uri"].get<std::string>(), std::ios::binary);
                std::streamsize total = 0;
                while (total < b.byteLength)
                {
                    bufferFile.read(reinterpret_cast<char*>(b.data.data()), b.byteLength - total);
                    total += bufferFile.gcount();
                }

                uint32_t magic = *reinterpret_cast<uint32_t*>(b.data.data());
                uint32_t version = *(reinterpret_cast<uint32_t*>(b.data.data()) + 1);
                uint32_t length = *(reinterpret_cast<uint32_t*>(b.data.data()) + 2);


                buffers.push_back(b);
            }

            for (auto const& anim : data["animations"])
            {
                Animation animation{};
                animation.name = anim["name"].get<std::string>();
                std::map<std::string, UINT, std::less<>> channelMap;

                for (auto const& chan : anim["channels"])
                {
                    UINT channel = 0;
                    UINT sampler = chan["sampler"].get<UINT>();
                    UINT node = chan["target"]["node"].get<UINT>();
                    std::string path = chan["target"]["path"].get<std::string>();

                    std::string nodeName = data["nodes"][node]["name"].get<std::string>();
                    if (channelMap.find(nodeName) != channelMap.end())
                    {
                        channel = channelMap[nodeName];
                    }
                    else
                    {
                        channel = channelMap.size();
                        channelMap.emplace(nodeName, channelMap.size());
                        AnimationNode c{};
                        c.nodeName = nodeName;
                        animation.nodes.push_back(c);
                    }
                        
                    auto const& samp = anim["samplers"][sampler];

                    UINT input = samp["input"].get<UINT>();
                    UINT output = samp["output"].get<UINT>();

                    auto const& timeAcc = data["accessors"][input];
                    auto const& valueAcc = data["accessors"][output];

                    auto const& timeBufferView = data["bufferViews"][timeAcc["bufferView"].get<UINT>()];
                    auto const& valueBufferView = data["bufferViews"][valueAcc["bufferView"].get<UINT>()];
                    UINT timeByteOffset = timeAcc["byteOffset"].get<UINT>();
                    timeByteOffset += timeBufferView["byteOffset"].get<UINT>();

                    UINT valueByteOffset = valueAcc["byteOffset"].get<UINT>();
                    valueByteOffset += valueBufferView["byteOffset"].get<UINT>();

                    UINT count = timeAcc["count"].get<UINT>();
                    Buffer& timeBuffer = buffers[timeBufferView["buffer"].get<UINT>()];
                    Buffer& valueBuffer = buffers[timeBufferView["buffer"].get<UINT>()];
                    for (UINT i = 0; i < count; i++)
                    {
                        AnimationNode& node = animation.nodes[channel];
                        if (path == "translation")
                        {
                            UINT index = (i * sizeof(float)) + timeByteOffset;
                            AnimationNode::VectorKey translation{};
                            memcpy(&translation.time, &timeBuffer.data[index], sizeof(float));
                            index = (i * sizeof(Vector3)) + valueByteOffset;
                            memcpy(&translation.value, &valueBuffer.data[index], sizeof(Vector3));
                            node.positionKeys.push_back(translation);
                        }
                        else if (path == "rotation")
                        {
                            UINT index = (i * sizeof(float)) + timeByteOffset;
                            AnimationNode::QuatKey rotation{};
                            memcpy(&rotation.time, &timeBuffer.data[index], sizeof(float));
                            index = (i * sizeof(Vector4)) + valueByteOffset;
                            memcpy(&rotation.value, &valueBuffer.data[index], sizeof(Vector4));
                            node.rotationKeys.push_back(rotation);
                        }
                        else if (path == "scale")
                        {
                            UINT index = (i * sizeof(float)) + timeByteOffset;
                            AnimationNode::VectorKey scale{};
                            memcpy(&scale.time, &timeBuffer.data[index], sizeof(float));
                            index = (i * sizeof(Vector3)) + valueByteOffset;
                            memcpy(&scale.value, &valueBuffer.data[index], sizeof(Vector3));
                            node.scalingKeys.push_back(scale);
                        }
                    }
                }

                sd.animations.push_back(animation);
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        
        return sd;
    }
}
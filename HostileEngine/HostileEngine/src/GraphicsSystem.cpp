#include "stdafx.h"
#include "GraphicsSystem.h"
#include "IGraphics.h"
#include "Engine.h"

#include <iostream>

#include "ResourceLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/Model.h>

namespace Hostile
{
    //void UpdateBones(
    //    float _animTime,
    //    Skeleton& _skeleton,
    //    Skeleton::Node& _node,
    //    Animation& _animation,
    //    const Matrix& _parentTransform
    //)
    //{
    //    Matrix nodeTransform = _node.transformation;
    //    AnimationNode* pAnimNode = nullptr;
    //    for (auto it = _animation.nodes.begin(); it != _animation.nodes.end(); ++it)
    //    {
    //        if (it->nodeName == _node.name)
    //        {
    //            pAnimNode = it._Ptr;
    //            break;
    //        }
    //    }
    //    if (pAnimNode)
    //    {
    //        Vector3 s = {};
    //        for (int i = 0; i < pAnimNode->scalingKeys.size() - 1; i++)
    //        {
    //            if (_animTime < pAnimNode->scalingKeys[i + 1].time)
    //            {
    //                float dt = pAnimNode->scalingKeys[i + 1].time - pAnimNode->scalingKeys[i].time;
    //                float factor = (_animTime - pAnimNode->scalingKeys[i].time) / dt;
    //                s = Vector3::Lerp(pAnimNode->scalingKeys[i].value, pAnimNode->scalingKeys[i + 1].value, factor);
    //                break;
    //            }
    //        }
    //        Matrix scale = Matrix::CreateScale(s);
    //
    //        Quaternion r = {};
    //        for (int i = 0; i < pAnimNode->rotationKeys.size() - 1; i++)
    //        {
    //            if (_animTime < pAnimNode->rotationKeys[i + 1].time)
    //            {
    //                float dt = pAnimNode->rotationKeys[i + 1].time - pAnimNode->rotationKeys[i].time;
    //                float factor = (_animTime - pAnimNode->rotationKeys[i].time) / dt;
    //                r = Quaternion::Lerp(pAnimNode->rotationKeys[i].value, pAnimNode->rotationKeys[i + 1].value, factor);
    //                break;
    //            }
    //        }
    //        Matrix rot = Matrix::CreateFromQuaternion(r);
    //
    //        Vector3 t = {};
    //        for (int i = 0; i < pAnimNode->positionKeys.size() - 1; i++)
    //        {
    //            if (_animTime < pAnimNode->positionKeys[i + 1].time)
    //            {
    //                float dt = pAnimNode->positionKeys[i + 1].time - pAnimNode->positionKeys[i].time;
    //                float factor = (_animTime - pAnimNode->positionKeys[i].time) / dt;
    //                t = Vector3::Lerp(pAnimNode->positionKeys[i].value, pAnimNode->positionKeys[i + 1].value, factor);
    //                break;
    //            }
    //        }
    //        Matrix trans = Matrix::CreateTranslation(t);
    //
    //        //nodeTransform = trans * (scale * rot);
    //        nodeTransform = XMMatrixTransformation(Vector3::Zero,
    //            Quaternion::Identity, s, Vector3::Zero, r, t);
    //        //nodeTransform = nodeTransform.Transpose();
    //    }
    //
    //    Matrix global = _parentTransform * nodeTransform;
    //
    //    if (_skeleton.boneMapping.find(_node.name) != _skeleton.boneMapping.end())
    //    {
    //        size_t boneIndex = _skeleton.boneMapping[_node.name];
    //        _skeleton.bones[boneIndex].finalTransform =
    //            global * _skeleton.bones[boneIndex].offset;
    //
    //        Matrix d = global;
    //        //IGraphics::Get().RenderDebug(d);
    //    }
    //
    //    for (auto& it : _node.children)
    //    {
    //        UpdateBones(_animTime, _skeleton, it, _animation, global);
    //    }
    //}
    //
    //void GetBoneTransforms(
    //    float _dt,
    //    Skeleton& _skeleton,
    //    Animation& _animation,
    //    std::vector<Matrix>& _bones
    //)
    //{
    //    _bones.resize(_skeleton.bones.size());
    //    _animation.timeInSeconds += _dt;
    //    float tics = _animation.ticksPerSec * _animation.timeInSeconds;
    //    tics = fmodf(tics, _animation.duration);
    //
    //    UpdateBones(tics, _skeleton, _skeleton.rootNode, _animation, Matrix::Identity);
    //    for (uint32_t i = 0; i < _bones.size(); i++)
    //    {
    //        _bones[i] = _skeleton.bones[i].finalTransform.Transpose();
    //    }
    //}

    //SceneData sd;
    //std::unique_ptr<MoltenVertexBuffer> vb;
    std::unique_ptr<MoltenTexture> mt;
    ADD_SYSTEM(GraphicsSys);
    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        auto p = IGraphics::Get().CreateGeometricPrimitive(GeometricPrimitive::CreateCube());
        m_meshes["Cube"] = std::move(p);
        _world.system<Transform, Mesh>().kind(flecs::OnUpdate).iter([&](flecs::iter& _info, Transform* _pTransforms, Mesh* _pMeshes) { OnUpdate(_info, _pTransforms, _pMeshes); });
        //_world.entity("cube01").add<Mesh>().set<Mesh>({ "Cube", 0 }).add<Transform>().set<Transform>({ Vector3(10, 0, 0) });
        //_world.entity("cube02").add<Mesh>().set<Mesh>({ "Cube", 0 }).add<Transform>().set<Transform>({ Vector3(0, 0, 0) });
        
        //sd = LoadSceneFromFile("Assets/models/export_test.fbx");
        //
        //std::vector<VertexPositionNormalTangentColorTextureSkinning> vertices;
        //for (auto& it : sd.meshData.vertices)
        //{
        //    VertexPositionNormalTangentColorTextureSkinning v;
        //    v.position = it.pos;
        //    v.normal = it.normal;
        //    v.textureCoordinate = it.texCoord;
        //    v.SetBlendIndices(it.m_bones);
        //    v.SetBlendWeights(it.weights);
        //    vertices.push_back(v);
        //}
        //
        //vb = IGraphics::Get().CreateVertexBuffer(
        //    vertices,
        //    sd.meshData.faces
        //);

        mt = IGraphics::Get().CreateTexture("grid");
    }

    void GraphicsSys::AddMesh(flecs::iter& _info)
    {
        
    }

    void GraphicsSys::AddTexture(flecs::iter& _info)
    {

    }

    void GraphicsSys::OnUpdate(flecs::iter& _info)
    {
        if (_info.is_set(1) && _info.is_set(2))
        {
            auto meshes = _info.field<Mesh>(1);
            auto textures = _info.field<Texture>(2);

            for (auto it : _info)
            {
                Mesh& mesh = meshes[it];
                Texture& texture = textures[it];
            }
        }
    }

    void GraphicsSys::OnUpdate(flecs::iter& _info, Transform* _pTransforms, Mesh* _pMeshes)
    {
        IGraphics& g = IGraphics::Get();
        //std::vector<Matrix> bones;
        //GetBoneTransforms(_info.delta_time(), sd.skeleton, sd.animations[0], bones);
        //Matrix m = Matrix::CreateScale(0.1f);
        //g.RenderVertexBuffer(vb, mt, bones, m);
        for (auto& it : _info)
        {
            Transform& t = _pTransforms[it];
            Mesh& m = _pMeshes[it];
            //Matrix mat = Matrix::CreateTranslation(t.position);
            Matrix& mat = _pTransforms[it].matrix;
            g.RenderGeometricPrimitive(m_meshes[m.meshName], mat);
        }
    }
}
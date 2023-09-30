#include "stdafx.h"
#include "GraphicsSystem.h"
#include "IGraphics.h"
#include "Engine.h"
#include "Rigidbody.h"

#include <iostream>

#include "ResourceLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/Model.h>

#include <imgui.h>

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
    ADD_SYSTEM(GraphicsSys);
    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        auto p = IGraphics::Get().CreateGeometricPrimitive(GeometricPrimitive::CreateCube());
        m_meshes.push_back(std::move(p));
        m_meshMap["Cube"] = m_meshes.size() - 1;
        auto t = IGraphics::Get().CreateTexture("grid");
        m_textures.push_back(std::move(t));
        m_textMap["grid"] = m_textures.size() - 1;
        //_world.system<Transform, Mesh>().kind(flecs::OnUpdate).without<Texture>().iter([&](flecs::iter& _info, Transform* _pTransforms, Mesh* _pMeshes) { OnUpdate(_info, _pTransforms, _pMeshes); });
        //_world.system<Transform, Mesh, Texture>().kind(flecs::OnUpdate).iter([&](flecs::iter& _info, Transform* _pTransforms, Mesh* _pMeshes, Texture* _pTextures) { OnUpdate(_info, _pTransforms, _pMeshes, _pTextures); });
        _world.system("PreRender").kind(flecs::PreUpdate).iter([&](flecs::iter& _info) { PreUpdate(_info); });
        _world.system<Transform, Mesh>("Render").kind(flecs::OnUpdate)
            .term<Texture>().optional().iter([&](flecs::iter& _info) { OnUpdate(_info); });
        _world.system("PostRender").kind(flecs::PostUpdate).iter([&](flecs::iter& _info) { PostUpdate(_info); });
        _world.entity("cube01").set<Mesh>({ "Cube", 0 }).set<Transform>({ Vector3(10, 0, 0) });
        _world.entity("cube02").set<Mesh>({ "Cube", 0 }).set<Transform>({ Vector3(0, 0, 0) }).set<Texture>({ "grid",0 });

        m_geometryPass = _world.query_builder<Transform, Mesh>().build();// .term<Texture>().optional().build();

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
        m_renderTargets.push_back(IGraphics::Get().CreateRenderTarget());

        m_camera.SetPerspective(45, 1920.0f / 1080.0f, 0.1f, 1000000);
        m_camera.LookAt({ 0, 0, 50 }, { 0, 0, 0 }, { 0, 1, 0 });
    }

    void GraphicsSys::PreUpdate(flecs::iter& _info)
    {
        IGraphics& g = IGraphics::Get();
        g.GetRenderContext()->GetEffect()->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());
        g.GetRenderContext()->SetRenderTarget(m_renderTargets[0]);
        g.GetRenderContext()->GetEffect()->SetDiffuseColor({ 1,1,1,1 });
        g.GetRenderContext()->GetEffect()->EnableDefaultLighting();
    }

    void GraphicsSys::AddMesh(flecs::iter& _info)
    {
    }

    void GraphicsSys::AddTexture(flecs::iter& _info)
    {
    }

    void GraphicsSys::OnUpdate(flecs::iter& _info)
    {
        auto transforms = _info.field<Transform>(1);
        auto meshes = _info.field<Mesh>(2);
        for (auto& it : m_renderTargets)
        {
            if (_info.is_set(3))
            {
                auto textures = _info.field<Texture>(3);
                OnUpdate(_info, transforms, meshes, textures);
            }
            else
            {
                OnUpdate(_info, transforms, meshes);
            }
        }
    }

    void GraphicsSys::OnUpdate(flecs::iter& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes)
    {
        std::shared_ptr<IRenderContext>& r = IGraphics::Get().GetRenderContext();
        //std::vector<Matrix> bones;
        //GetBoneTransforms(_info.delta_time(), sd.skeleton, sd.animations[0], bones);
        //Matrix m = Matrix::CreateScale(0.1f);
        //g.RenderVertexBuffer(vb, mt, bones, m);
        for (size_t it : _info)
        {
            Transform& t = _pTransforms[it];
            Mesh& m = _pMeshes[it];
            Matrix mat = Matrix::CreateTranslation(t.position);
            auto& e = _info.entity(it);
            if (e.has<Rigidbody>())
            {
                //mat = *e.get<Matrix>();
                mat = t.matrix;
            }
            r->RenderGeometricPrimitive(*m_meshes[m.meshIndex], std::move(mat));
        }
    }

    void GraphicsSys::OnUpdate(flecs::iter& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes, flecs::column<Texture>& _pTextures)
    {
        std::shared_ptr<IRenderContext>& r = IGraphics::Get().GetRenderContext();
        for (size_t it : _info)
        {
            Transform& t = _pTransforms[it];
            Mesh& m = _pMeshes[it];
            Matrix mat = Matrix::CreateTranslation(t.position);
            Texture& tex = _pTextures[it];
            auto& e = _info.entity(it);
            if (e.has<Rigidbody>())
            {
                //mat = *e.get<Matrix>();
                mat = t.matrix;
            }
            r->RenderGeometricPrimitive(*m_meshes[m.meshIndex], *m_textures[tex.textureIndex], std::move(mat));
        }
    }

    void GraphicsSys::PostUpdate(flecs::iter& _info)
    {
        IGraphics::Get().ExecuteRenderContext(IGraphics::Get().GetRenderContext());

        ImGui::Begin("View");
        if (ImGui::IsWindowFocused() && ImGui::IsWindowDocked())
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            float testDelta = ImGui::GetIO().MouseWheel;

            if (dragDelta.x == 0 && dragDelta.y == 0)
            {
                m_currDragDelta = { dragDelta.x, dragDelta.y };               //im sam and i'm black
            }

            float x = dragDelta.x - m_currDragDelta.x;
            float y = dragDelta.y - m_currDragDelta.y;
            Vector3 pos = m_camera.GetPosition();
            //m_camera.Pitch(-y * 0.1f);
            m_camera.MoveUp(y * 0.2f);
            m_currDragDelta = { dragDelta.x, dragDelta.y };
            //m_camera.Yaw(-x * 0.1f);
            m_camera.MoveRight(x * 0.2f);
            Vector3 tpos = m_camera.GetPosition();
            tpos.Normalize();
            pos = tpos * pos.Length();
            m_camera.SetPosition(pos);
            if (testDelta >= 1 || testDelta <= -1)
            {
                m_camera.MoveForward(testDelta);
            }

            m_camera.LookAt(m_camera.GetPosition(), { 0, 0, 0 }, { 0, 1, 0 });
        }
        D3D12_VIEWPORT vp = m_renderTargets[0]->vp;
        float aspect = vp.Width / vp.Height;
        float inverseAspect = vp.Height / vp.Width;
        ImVec2 imageSize(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        if ((imageSize.y * aspect) > imageSize.x)
        {
            imageSize.y = imageSize.x * inverseAspect;
        }
        else
        {
            imageSize.x = imageSize.y * aspect;
        }
        ImVec2 cursorPos = ImGui::GetWindowSize();
        cursorPos.x = (cursorPos.x - imageSize.x) * 0.5f;
        cursorPos.y = (cursorPos.y - imageSize.y) * 0.5f;
        ImGui::SetCursorPos(cursorPos);
        if (m_renderTargets[0]->currentState[(m_renderTargets[0]->frameIndex + 1) % FRAME_COUNT] == D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
        {
            ImGui::Image(
                (ImTextureID)m_renderTargets[0]->srv[(m_renderTargets[0]->frameIndex + 1) % FRAME_COUNT].ptr,
                imageSize
            );
        }

        ImGui::End();
    }
}
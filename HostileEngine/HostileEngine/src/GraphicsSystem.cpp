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

#include "Input.h"
namespace Hostile
{
    void UpdateBones(
        float _animTime,
        SceneData& _scene,
        Node const& _node,
        UINT _nodeIndex,
        Animation& _animation,
        const Matrix& _parentTransform
    )
    {
        Matrix nodeTransform = XMMatrixTransformation(
            Vector3::Zero,
            Quaternion::Identity,
            _node.scale,
            Vector3::Zero,
            _node.rotation,
            _node.translation
        );
        AnimationNode* pAnimNode = nullptr;

        for (auto it = _animation.nodes.begin(); it != _animation.nodes.end(); ++it)
        {
            if (it->nodeName == _node.name)
            {
                pAnimNode = it._Ptr;
                break;
            }
        }
        //L_elbow_ctrl

        if (pAnimNode)
        {
            Vector3 s = _node.scale;
            for (int i = 0; i < (int)pAnimNode->scalingKeys.size() - 1; i++)
            {
                if (_animTime < pAnimNode->scalingKeys[i + 1].time)
                {
                    float dt = (pAnimNode->scalingKeys[i + 1].time - pAnimNode->scalingKeys[i].time);
                    float factor = (_animTime - pAnimNode->scalingKeys[i].time) / dt;
                    s = Vector3::Lerp(pAnimNode->scalingKeys[i].value, pAnimNode->scalingKeys[i + 1].value, factor);
                    break;
                }
            }

            Quaternion r = _node.rotation;
            for (int i = 0; i < (int)pAnimNode->rotationKeys.size() - 1; i++)
            {
                if (_animTime < pAnimNode->rotationKeys[i + 1].time)
                {
                    float dt = (pAnimNode->rotationKeys[i + 1].time - pAnimNode->rotationKeys[i].time);
                    float factor = (_animTime - pAnimNode->rotationKeys[i].time) / dt;
                    r = Quaternion::Lerp(pAnimNode->rotationKeys[i].value, pAnimNode->rotationKeys[i + 1].value, factor);
                    r.Normalize();
                    break;
                }
            }

            Vector3 t = _node.translation;
            for (int i = 0; i < (int)pAnimNode->positionKeys.size() - 1; i++)
            {
                if (_animTime < pAnimNode->positionKeys[i + 1].time)
                {
                    float dt = (pAnimNode->positionKeys[i + 1].time - pAnimNode->positionKeys[i].time);

                    float factor = (_animTime - pAnimNode->positionKeys[i].time) / dt;

                    t = Vector3::Lerp(pAnimNode->positionKeys[i].value, pAnimNode->positionKeys[i + 1].value, factor);
                    break;
                }
            }


            nodeTransform = XMMatrixTransformation(
                Vector3::Zero,
                Quaternion::Identity, s, Vector3::Zero, r, t);
        }

        Matrix global = nodeTransform * _parentTransform;

        int boneIndex = -1;
        for (UINT i = 0; i < _scene.skeleton.joints.size(); i++)
        {
            if (_scene.skeleton.joints[i] == _nodeIndex)
            {
                boneIndex = i;
                break;
            }
        }
        if (boneIndex != -1)
            _scene.skeleton.boneMatrices[boneIndex] = _scene.skeleton.inverseBindMatrices[boneIndex] * global;

        for (auto const& it : _node.children)
        {
            UpdateBones(_animTime, _scene, _scene.nodes[it], it, _animation, global);
        }
    }

    void GetBoneTransforms(
        float _dt,
        SceneData& _scene,
        std::vector<Matrix>& _bones
    )
    {
        _bones.resize(_scene.skeleton.joints.size());
        Animation& animation = _scene.animations[1];
        animation.timeInSeconds += _dt;
        animation.duration = 1.625f;
        if (animation.timeInSeconds > animation.duration)
            animation.timeInSeconds -= animation.duration;

        UpdateBones(animation.timeInSeconds, _scene, _scene.nodes[_scene.skeleton.skeleton], _scene.skeleton.skeleton, animation, Matrix::Identity);
        _bones = _scene.skeleton.boneMatrices;
    }

    SceneData sd;
    std::unique_ptr<VertexBuffer> vb;
    ADD_SYSTEM(GraphicsSys);
    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        auto p = IGraphics::Get().CreateGeometricPrimitive(GeometricPrimitive::CreateSphere());


        m_meshes.push_back(std::move(p));
        m_meshMap["Cube"] = m_meshes.size() - 1;
        auto t = IGraphics::Get().CreateTexture("grid");
        m_textures.push_back(std::move(t));
        m_textMap["grid"] = m_textures.size() - 1;
        _world.system<Mesh>("OnMeshCreate").kind(flecs::OnAdd).iter([this](flecs::iter& _info) { AddMesh(_info); });
        _world.system("PreRender").kind(flecs::PreUpdate).iter([this](flecs::iter const& _info) { PreUpdate(_info); });

        _world.system("Render").kind(flecs::OnUpdate).iter([this](flecs::iter const& _info) { OnUpdate(_info); });

        _world.system("PostRender").kind(flecs::PostUpdate).iter([this](flecs::iter const& _info) { PostUpdate(_info); });

        _world.entity("cube01").set<Mesh>({ "Cube", 0 }).set<Transform>({ Vector3(10, 0, 0) });
        auto& e = _world.entity("cube02").set<Mesh>({ "Cube", 0 }).set<Transform>({ Vector3(0, 0, 0) }).set<Texture>({ "grid",0 });

        m_geometryPass = _world.query_builder<Transform, Mesh, Material>().build();

        sd = LoadSceneFromFile(std::string{ "Assets/models/Bear_out/Bear.gltf" });
        sd.animations = LoadAnimationFromFile(std::string{ "Assets/models/Bear_WalkForward_out/Bear_WalkForward.gltf" }).animations;

        vb = IGraphics::Get().CreateVertexBuffer(
            sd.meshData.vertices,
            sd.meshData.indices
        );
        m_renderTargets.push_back(IGraphics::Get().CreateRenderTarget());
        m_depthTargets.push_back(IGraphics::Get().CreateDepthTarget());

        m_camera.SetPerspective(45, 1920.0f / 1080.0f, 0.1f, 1000000);
        m_camera.LookAt({ 0, 0, 50 }, { 0, 0, 0 }, { 0, 1, 0 });
    }

    void GraphicsSys::PreUpdate(flecs::iter const& _info)
    {
        IGraphics& g = IGraphics::Get();
        g.GetRenderContext()->GetEffect()->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());
        g.GetRenderContext()->SetRenderTarget(m_renderTargets[0], m_depthTargets[0]);
        g.GetRenderContext()->GetEffect()->SetDiffuseColor({ 1,1,1,1 });
        g.GetRenderContext()->GetEffect()->EnableDefaultLighting();
        g.GetRenderContext()->GetStencilEffect()->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());

        std::shared_ptr<IRenderContext>& r = IGraphics::Get().GetRenderContext();
        r->GetSkinnedEffect()->SetMatrices(Matrix::Identity, m_camera.View(), m_camera.Projection());
        r->GetSkinnedEffect()->EnableDefaultLighting();
        std::vector<Matrix> bones;
        GetBoneTransforms(_info.delta_time(), sd, bones);
        Matrix m = Matrix::CreateScale(1.1f);
        r->RenderVertexBuffer(
            *vb,
            *m_textures[0],
            bones,
            m
        );
    }

    void GraphicsSys::AddMesh(flecs::iter& _info)
    {
        // TODO
        for (auto& it : _info)
        {
            _info.entity(it).add<Material>();
        }
    }

    void GraphicsSys::AddTexture(flecs::iter& _info)
    {
        // TODO
    }

    void GraphicsSys::OnUpdate(flecs::iter const& _info)
    {
        /*auto transforms = _info.field<Transform>(1);
        auto meshes = _info.field<Mesh>(2);
        for (auto const& it : m_renderTargets)
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
        }*/

        m_geometryPass.each([this](Transform& _transform, Mesh& _mesh, Material& _material) { OnUpdate(_transform, _mesh, _material); });
    }

    void GraphicsSys::OnUpdate(flecs::iter const& _info, flecs::column<Transform>& _pTransforms, flecs::column<Mesh>& _pMeshes)
    {
        std::shared_ptr<IRenderContext>& r = IGraphics::Get().GetRenderContext();

        for (size_t it : _info)
        {
            Transform& t = _pTransforms[it];
            Mesh& m = _pMeshes[it];

            r->RenderGeometricPrimitive(*m_meshes[m.meshIndex], t.matrix);
        }
    }

    void GraphicsSys::OnUpdate(Transform& _transform, Mesh& _mesh, Material& _material)
    {
        std::shared_ptr<IRenderContext>& r = IGraphics::Get().GetRenderContext();
        //for (size_t it : _info)
        {                              
            if (_material.textureIndex != -1)
            {
                r->RenderGeometricPrimitive(*m_meshes[_mesh.meshIndex], *m_textures[_material.textureIndex], _transform.matrix);
            }
            else
            {
                r->RenderGeometricPrimitive(*m_meshes[_mesh.meshIndex], _transform.matrix);
            }
        }
    }

    void GraphicsSys::PostUpdate(flecs::iter const& _info)
    {
        IGraphics::Get().ExecuteRenderContext(IGraphics::Get().GetRenderContext());

        ImGui::Begin("View");
        if (ImGui::IsWindowFocused() && ImGui::IsWindowDocked())
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

            if (dragDelta.x == 0 && dragDelta.y == 0)
            {
                m_currDragDelta = { dragDelta.x, dragDelta.y };
            }

            float x = dragDelta.x - m_currDragDelta.x;
            float y = dragDelta.y - m_currDragDelta.y;
            m_camera.Pitch(y * _info.delta_time() * 5);
            m_currDragDelta = { dragDelta.x, dragDelta.y };
            m_camera.Yaw(x * _info.delta_time() * -5);
            
            if (Input::IsPressed(Key::W))
                m_camera.MoveForward(_info.delta_time() * 5);
            if (Input::IsPressed(Key::S))
                m_camera.MoveForward(_info.delta_time() * -5);
            if (Input::IsPressed(Key::A))
                m_camera.MoveRight(_info.delta_time() * 5);
            if (Input::IsPressed(Key::D))
                m_camera.MoveRight(_info.delta_time() * -5);
            if (Input::IsPressed(Key::E))
                m_camera.MoveUp(_info.delta_time() * 5);
            if (Input::IsPressed(Key::Q))
                m_camera.MoveUp(_info.delta_time() * -5);
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
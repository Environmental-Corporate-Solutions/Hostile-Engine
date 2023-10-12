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
#include <filesystem>

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
            for (size_t i = 0; (i + 1) < pAnimNode->scalingKeys.size(); i++)
            {
                size_t nextIndex = (i + 1);
                if (_animTime < pAnimNode->scalingKeys[nextIndex].time)
                {
                    float dt = (pAnimNode->scalingKeys[nextIndex].time - pAnimNode->scalingKeys[i].time);
                    float factor = (_animTime - pAnimNode->scalingKeys[i].time) / dt;
                    s = Vector3::Lerp(pAnimNode->scalingKeys[i].value, pAnimNode->scalingKeys[nextIndex].value, factor);
                    break;
                }
            }

            Quaternion r = _node.rotation;
            for (size_t i = 0; (i + 1) < pAnimNode->rotationKeys.size() - 1; i++)
            {
                size_t nextIndex = (i + 1);
                if (_animTime < pAnimNode->rotationKeys[nextIndex].time)
                {
                    float dt = (pAnimNode->rotationKeys[nextIndex].time - pAnimNode->rotationKeys[i].time);
                    float factor = (_animTime - pAnimNode->rotationKeys[i].time) / dt;
                    r = Quaternion::Lerp(pAnimNode->rotationKeys[i].value, pAnimNode->rotationKeys[nextIndex].value, factor);
                    r.Normalize();
                    break;
                }
            }

            Vector3 t = _node.translation;
            for (size_t i = 0; (i + 1) < pAnimNode->positionKeys.size() - 1; i++)
            {
                size_t nextIndex = (i + 1);
                if (_animTime < pAnimNode->positionKeys[nextIndex].time)
                {
                    float dt = (pAnimNode->positionKeys[nextIndex].time - pAnimNode->positionKeys[i].time);

                    float factor = (_animTime - pAnimNode->positionKeys[i].time) / dt;

                    t = Vector3::Lerp(pAnimNode->positionKeys[i].value, pAnimNode->positionKeys[nextIndex].value, factor);
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

    ADD_SYSTEM(GraphicsSys);
    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        REGISTER_TO_SERIALIZER(InstanceID, this);
        // Meshes
        IGraphics& graphics = IGraphics::Get();
        m_meshMap.try_emplace("Cube", graphics.LoadMesh("Cube"));
        m_meshMap.try_emplace("Sphere", graphics.LoadMesh("Sphere"));

        _world.system("PreRender").kind(flecs::PreUpdate).iter([this](flecs::iter const& _info) { PreUpdate(_info); });

        _world.system("Render").kind(flecs::OnUpdate).iter([this](flecs::iter const& _info) { OnUpdate(_info); });

        _world.system("PostRender").kind(flecs::PostUpdate).iter([this](flecs::iter const& _info) { PostUpdate(_info); });


        Transform t{};
        t.position = Vector3{ 0, 0, 0 };
        t.scale = Vector3{ 100, 1, 100 };
        t.orientation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f);
        t.matrix = Matrix::CreateTranslation(0, 0, 0);
        MaterialID defaultMaterial = graphics.CreateMaterial(std::string("Default"));

        InstanceID planeid = graphics.CreateInstance(m_meshMap["Cube"], defaultMaterial);
        auto& plane = _world.entity("Plane");

        plane.set<Transform>(t)
            .set<InstanceID>(planeid);
        _world.entity("box1").set<InstanceID>(graphics.CreateInstance(m_meshMap["Cube"], defaultMaterial));
        _world.entity("box2").set<InstanceID>(graphics.CreateInstance(m_meshMap["Cube"], defaultMaterial));
        _world.entity("Sphere2").set<InstanceID>(graphics.CreateInstance(m_meshMap["Sphere"], defaultMaterial));

        //for (size_t y = 0; y < 10; y++)
        //{
        //    for (size_t x = 0; x < 10; x++)
        //    {
        //        MaterialID material = graphics.CreateMaterial(std::string("TestMaterial" + std::to_string(x * y + x)));
        //        InstanceID id = graphics.CreateInstance(m_meshMap["Cube"], material);
        //        _world.entity(std::string("cube" + std::to_string(x * y + x)).c_str())
        //            .set<Transform>(t)
        //            .set<InstanceID>(id);
        //    }
        //}


        m_geometryPass = _world.query_builder<InstanceID, Transform>().build();
        m_renderTargets.push_back(IGraphics::Get().CreateRenderTarget());
        m_depthTargets.push_back(IGraphics::Get().CreateDepthTarget());

        m_camera.SetPerspective(45, 1920.0f / 1080.0f, 0.1f, 1000000);
        m_camera.LookAt({ 0, 5, 10 }, { 0, 0, 0 }, { 0, 1, 0 });
    }

    void GraphicsSys::PreUpdate(flecs::iter const&)
    {
        // currently unused
    }

    void GraphicsSys::AddMesh(flecs::iter& _info)
    {
        // TODO
    }

    void GraphicsSys::AddTexture(flecs::iter& _info)
    {
        // TODO
    }

    void GraphicsSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components)
    {
      //const Mesh& mesh = *_entity.get<Mesh>();
      //nlohmann::json obj = nlohmann::json::object();
      //obj["Type"] = "Mesh";
      //obj["Mesh Name"] = mesh.meshName;
      //obj["Mesh Index"] = mesh.meshIndex;
      //_components.push_back(obj);

    }

    void GraphicsSys::Read(nlohmann::json object)
    {
    }

    void GraphicsSys::GuiDisplay(flecs::entity& _entity)
    {
    }

    void GraphicsSys::OnUpdate(flecs::iter const&) const
    {
        m_renderTargets[0]->SetCameraPosition(m_camera.GetPosition());
        m_renderTargets[0]->SetView(m_camera.View());
        m_renderTargets[0]->SetProjection(m_camera.Projection());

        IGraphics& graphics = IGraphics::Get();
        m_geometryPass.each(
            [&graphics](InstanceID const& _instance, Transform const& _transform)
            {
                graphics.UpdateInstance(_instance, _transform.matrix);
            });
    }

    void GraphicsSys::OnUpdate(InstanceID const& _instance, Transform const& _transform) const
    {
        IGraphics::Get().UpdateInstance(_instance, _transform.matrix);
    }

    void GraphicsSys::PostUpdate(flecs::iter const& _info)
    {
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

        Vector2 vp = m_renderTargets[0]->GetDimensions();
        float aspect = vp.x / vp.y;
        float inverseAspect = vp.y / vp.x;
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

        ImGui::Image(
            (ImTextureID)m_renderTargets[0]->GetPtr(),
            imageSize
        );

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB", ImGuiDragDropFlags_None))
            {
                std::string path = *static_cast<std::string*>(payload->Data);
                //std::string thePath = entry.path().string();
                flecs::entity thing = IEngine::Get().GetWorld().entity();
                thing.from_json(path.c_str());
                Log::Trace(thing.name());
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::End();
    }
}
#include "stdafx.h"
#include "GraphicsSystem.h"
#include "IGraphics.h"
#include "Engine.h"
#include "Rigidbody.h"
#include "Deseralizer.h"

#include <iostream>

#include "ResourceLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <directxtk12/Model.h>

#include <imgui.h>
#include <filesystem>

#include "Input.h"
#include "font_awesome.h"
#include "Script/ScriptSys.h"
#include "ImGuizmo.h"
#include "TransformSys.h"

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

    InstanceData GraphicsSys::ConstructInstance(const std::string _mesh, const std::string _material)
    {
        InstanceData instance{};
        instance.meshName = _mesh;
        instance.materialName = _material;

        instance.meshId = m_meshMap[_mesh];
        instance.materialId = m_materialMap[_material];

        instance.id = IGraphics::Get().CreateInstance(instance.meshId, instance.materialId);
        return instance;
    }

    ADD_SYSTEM(GraphicsSys);

    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        REGISTER_TO_SERIALIZER(InstanceID, this);
        IEngine::Get().GetGUI().RegisterComponent("InstanceData",
            std::bind(&GraphicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
            [](flecs::entity& _entity) {_entity.add<InstanceData>(); });
        // Meshes
        IGraphics& graphics = IGraphics::Get();
        m_meshMap.try_emplace("Cube", graphics.LoadMesh("Cube"));
        m_meshMap.try_emplace("Sphere", graphics.LoadMesh("Sphere"));

        _world.system("PreRender").kind(flecs::PreUpdate).iter([this](flecs::iter const& _info) { PreUpdate(_info); });

        _world.system("Render").kind(flecs::OnUpdate).iter([this](flecs::iter const& _info) { OnUpdate(_info); });

        _world.system("PostRender").kind(flecs::PostUpdate).iter([this](flecs::iter const& _info) { PostUpdate(_info); });


        graphics.LoadPipeline("Default");
        graphics.LoadPipeline("Skybox");
        Transform t{};
        t.position = Vector3{ 0, 0, 0 };
        t.scale = Vector3{ 100, 1, 100 };
        t.orientation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f);
        t.matrix = Matrix::CreateTranslation(0, 0, 0);
        m_materialMap["Default"] = graphics.LoadMaterial("Default", "Default");
        m_materialMap["EmmissiveWhite"] = graphics.LoadMaterial("EmmissiveWhite", "Default");
        m_materialMap["EmmissiveRed"] = graphics.LoadMaterial("EmmissiveRed", "Default");
        m_materialMap["Skybox"] = graphics.LoadMaterial("Skybox", "Skybox");
        _world.entity("Skybox").set<InstanceData>(ConstructInstance("Cube", "Skybox")).set<Transform>(t);
        auto& plane = _world.entity("Plane");

        plane.set<Transform>(t)
            .set<InstanceData>(ConstructInstance("Cube", "Default"));
        _world.entity("box1").set<InstanceData>(ConstructInstance("Cube", "Default"));
        _world.entity("box2").set<InstanceData>(ConstructInstance("Cube", "Default"));
        _world.entity("box3").set<InstanceData>(ConstructInstance("Cube", "Default"));
        _world.entity("Sphere1").set<InstanceData>(ConstructInstance("Sphere", "Default"));
        _world.entity("Sphere2").set<InstanceData>(ConstructInstance("Sphere", "Default"));
        _world.entity("Sphere3").set<InstanceData>(ConstructInstance("Sphere", "Default"));
        _world.entity("Sphere4").set<InstanceData>(ConstructInstance("Sphere", "Default"));

        LightData lightData{};
        lightData.color = Vector3{ 1, 1, 1 };
        lightData.id = graphics.CreateLight();
        t.position = Vector3{ 18, 2, 10 };
        t.scale = Vector3{ 1, 1, 1 };

        _world.entity("Light").set<InstanceData>(ConstructInstance("Sphere", "EmmissiveWhite"))
            .set<Transform>(t)
            .set<LightData>(lightData);


        m_geometryPass = _world.query_builder<InstanceData, Transform>().build();
        m_lightPass = _world.query_builder<LightData, Transform>().build();

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

    void GraphicsSys::OnUpdate(flecs::iter const&) const
    {
        m_renderTargets[0]->SetCameraPosition(m_camera.GetPosition());
        m_renderTargets[0]->SetView(m_camera.View());
        m_renderTargets[0]->SetProjection(m_camera.Projection());

        IGraphics& graphics = IGraphics::Get();
        m_geometryPass.each(
            [&graphics](InstanceData const& _instance, Transform const& _transform)
            {
                graphics.UpdateInstance(_instance.id, _transform.matrix);
            });

        m_lightPass.each(
            [&graphics](LightData const& _light, Transform const& _transform)
            {
                graphics.UpdateLight(_light.id, _transform.position, _light.color);
            });
    }

    void GraphicsSys::OnUpdate(InstanceData const& _instance, Transform const& _transform) const
    {
        IGraphics::Get().UpdateInstance(_instance.id, _transform.matrix);
    }

    void GraphicsSys::PostUpdate(flecs::iter const& _info)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
        ImGui::Begin("View");
        if (ImGui::IsWindowFocused() && ImGui::IsWindowDocked())
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

            if (dragDelta.x == 0 && dragDelta.y == 0)
            {
                m_currDragDelta = { dragDelta.x, dragDelta.y };
            }

            float x = dragDelta.x - m_currDragDelta.x;
            float y = dragDelta.y - m_currDragDelta.y;
            m_camera.Pitch(y * _info.delta_time() * 5);
            m_currDragDelta = { dragDelta.x, dragDelta.y };
            m_camera.Yaw(x * _info.delta_time() * -5);
            float speed = 5;
            if (Input::IsPressed(Key::LeftShift))
            {
                speed *= 3;
            }
            else
            {
                speed = 5;
            }

            if (Input::IsPressed(Key::W))
                m_camera.MoveForward(_info.delta_time() * speed);
            if (Input::IsPressed(Key::S))
                m_camera.MoveForward(_info.delta_time() * -speed);
            if (Input::IsPressed(Key::A))
                m_camera.MoveRight(_info.delta_time() * speed);
            if (Input::IsPressed(Key::D))
                m_camera.MoveRight(_info.delta_time() * -speed);
            if (Input::IsPressed(Key::E))
                m_camera.MoveUp(_info.delta_time() * speed);
            if (Input::IsPressed(Key::Q))
                m_camera.MoveUp(_info.delta_time() * -speed);
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
                IDeseralizer::Get().ReadFile(path.c_str());
                ImGui::EndDragDropTarget();
            }
        }

        int objId = IEngine::Get().GetGUI().GetSelectedObject();
        if (objId != -1)
        {
            flecs::entity& current = IEngine::Get().GetWorld().entity(objId);
            Transform& transform = *current.get_mut<Transform>();
            
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

        	{   //compute viewport for ImGuizmo

                ImVec2 min = cursorPos;
                ImVec2 pos = ImGui::GetWindowPos();
                min += pos;

                //shows a box of where the gizmo will be drawn on
                //ImGui::GetForegroundDrawList()->AddRect(min, min + imageSize, ImColor(0, 255, 0));

                ImGuizmo::SetRect(min.x, min.y, imageSize.x, imageSize.y);
            }

            SimpleMath::Matrix matrix = transform.matrix;
            ImGuizmo::Manipulate(&(m_camera.View().m[0][0]), &(m_camera.Projection().m[0][0]), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, &matrix.m[0][0]);

            if (ImGuizmo::IsUsingAny()) //compute only when we modify 
            {
                Vector3 euler;
                ImGuizmo::DecomposeMatrixToComponents(&matrix.m[0][0], &transform.position.x, &euler.x, &transform.scale.x);
                euler *= PI / 180.0f;
                transform.orientation = Quaternion::CreateFromYawPitchRoll(euler);
            }
        }


        ImGui::End();
        ImGui::PopStyleVar();
    }

    void GraphicsSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
    {
        using namespace nlohmann;
        const InstanceData* data = _entity.get<InstanceData>();
        json obj = json::object();
        obj["Type"] = "InstanceData";
        obj["Mesh"] = data->meshName;
        obj["Material"] = data->materialName;
        _components.push_back(obj);
    }

    void GraphicsSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
    {
    }

    void GraphicsSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
    {
        if (_entity.has<InstanceData>())
        {
            InstanceData* data = _entity.get_mut<InstanceData>();

            if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("###mesh", data->meshName.data()))
                {
                    for (const auto& [name, id] : m_meshMap)
                    {
                        bool selected = (id == data->meshId);
                        if (ImGui::Selectable(name.c_str(), &selected))
                        {
                            data->meshName = name;
                            data->meshId = id;
                            IGraphics::Get().UpdateInstance(data->id, data->meshId);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("###material", data->materialName.data()))
                {
                    for (const auto& [name, id] : m_materialMap)
                    {
                        bool selected = (id == data->materialId);
                        if (ImGui::Selectable(name.c_str(), &selected))
                        {
                            data->materialName = name;
                            data->materialId = id;
                            IGraphics::Get().UpdateInstance(data->id, data->materialId);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Edit"))
                    ImGui::OpenPopup("Material Editor");

                IGraphics::Get().ImGuiMaterialPopup(data->materialId);
                ImGui::TreePop();
            }

        }

        if (_entity.has<LightData>())
        {
            LightData* data = _entity.get_mut<LightData>();
            if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorPicker3("Color", &data->color.x);
                ImGui::TreePop();
            }
        }
    }
}
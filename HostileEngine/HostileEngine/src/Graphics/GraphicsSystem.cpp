#include "stdafx.h"
#include "GraphicsSystem.h"
#include "IGraphics.h"
#include "Engine.h"
#include "PhysicsProperties.h"
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
#include "Script/ScriptSys.h"
#include "ImGuizmo.h"
#include "TransformSys.h"
#include "CameraComponent.h"
#include <misc/cpp/imgui_stdlib.h>

#include "RenderTarget.h"

#include <typeinfo>

#include "Resources/Pipeline.h"
#include "Resources/Material.h"

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

    InstanceData GraphicsSys::ConstructInstance(const std::string _mesh, const std::string _material, const UINT32 _id)
    {
        InstanceData instance{};
        instance.m_id = _id;

        instance.m_vertex_buffer = m_mesh_map[_mesh];
        instance.m_material = m_material_map[_material];
        instance.m_stencil = 0;
        return instance;
    }

    ADD_SYSTEM(GraphicsSys);

    void GraphicsSys::OnCreate(flecs::world& _world)
    {
        REGISTER_TO_SERIALIZER(InstanceData, this);
        REGISTER_TO_SERIALIZER(LightData, this);
        REGISTER_TO_DESERIALIZER(InstanceData, this);
        REGISTER_TO_DESERIALIZER(LightData, this);
        IEngine::Get().GetGUI().RegisterComponent("InstanceData",
            std::bind(&GraphicsSys::GuiDisplay, 
                this, std::placeholders::_1, std::placeholders::_2),
            [this](flecs::entity& _entity) 
            { _entity.set<InstanceData>(ConstructInstance(
                "Cube", "Default", _entity.id())); });

        IEngine::Get().GetGUI().RegisterComponent(
            "LightData",
            std::bind(&GraphicsSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
            [this](flecs::entity& _entity) { _entity.set<LightData>({ {1, 1, 1}, light_id++ }); });

        // Meshes
        IGraphics& graphics = IGraphics::Get();
        ResourceLoader& loader = ResourceLoader::Get();
        m_mesh_map["Cube"]   = loader.GetOrLoadResource<VertexBuffer>("Cube");
        m_mesh_map["Sphere"] = loader.GetOrLoadResource<VertexBuffer>("Sphere");
        //m_outline_buffer = graphics.GetOrLoadMesh("Square");
        //m_outline_pipeline = graphics.GetOrLoadPipeline("Outline");
        //m_outline_material = graphics.GetOrLoadMaterial("Outline");
        //m_outline_material->SetPipeline(m_outline_pipeline);

        _world.system("Editor PreRender").kind(flecs::PreUpdate).kind<Editor>().iter([this](flecs::iter const& _info) { PreUpdate(_info); });
        _world.system("Editor Render").kind(flecs::OnUpdate).kind<Editor>().iter([this](flecs::iter const& _info) { OnUpdate(_info); });
        _world.system("Editor PostRender").kind(flecs::PostUpdate).kind<Editor>().iter([this](flecs::iter const& _info) { PostUpdate(_info); });

        _world.system("PreRender").kind(flecs::PreUpdate).iter([this](flecs::iter const& _info) { PreUpdate(_info); });
        _world.system("Render").kind(flecs::OnUpdate).iter([this](flecs::iter const& _info) { OnUpdate(_info); });
        _world.system("PostRender").kind(flecs::PostUpdate).iter([this](flecs::iter const& _info) { PostUpdate(_info); });


        loader.GetOrLoadResource<Pipeline>("Assets/Pipelines/Default.json");
        loader.GetOrLoadResource<Pipeline>("Assets/Pipelines/Skybox.json");
        Transform t{};
        t.position = Vector3{ 0, 0, 0 };
        t.scale = Vector3{ 100, 1, 100 };
        t.orientation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f);
        t.matrix = Matrix::CreateTranslation(0, 0, 0);
        
        m_material_map["Default"]        = loader.GetOrLoadResource<Material>("Assets/materials/Default.mat");
        m_material_map["EmmissiveWhite"] = loader.GetOrLoadResource<Material>("Assets/materials/EmissiveWhite.mat");
        m_material_map["EmmissiveRed"]   = loader.GetOrLoadResource<Material>("Assets/materials/EmissiveRed.mat");
        m_material_map["Skybox"]         = loader.GetOrLoadResource<Material>("Assets/materials/Skybox.mat");
        
        auto e = _world.entity("Skybox"); 
        e.set<InstanceData>(ConstructInstance("Cube", "Skybox", e.id())).set<Transform>(t);
        e.set<ObjectName>({ "Skybox" });
        
        auto& plane = _world.entity("Plane");

        plane.set<Transform>(t).set<InstanceData>(ConstructInstance("Cube", "Default", plane.id()));

        e = _world.entity("box1"); e.set<InstanceData>(ConstructInstance("Cube", "Default", e.id()));
        e = _world.entity("box2"); e.set<InstanceData>(ConstructInstance("Cube", "Default", e.id()));
        e = _world.entity("box3"); e.set<InstanceData>(ConstructInstance("Cube", "Default", e.id()));
        e = _world.entity("Sphere1"); e.set<InstanceData>(ConstructInstance("Sphere", "Default", e.id()));
        e = _world.entity("Sphere2"); e.set<InstanceData>(ConstructInstance("Sphere", "Default", e.id()));
        e = _world.entity("Sphere3"); e.set<InstanceData>(ConstructInstance("Sphere", "Default", e.id()));
        e = _world.entity("Sphere4"); e.set<InstanceData>(ConstructInstance("Sphere", "Default", e.id()));

        LightData lightData{};
        lightData.color = Vector3{ 1, 1, 1 };
        lightData.id = 0;
        t.position = Vector3{ 18, 2, 10 };
        t.scale = Vector3{ 1, 1, 1 };

        e = _world.entity("Light");
        e.set<InstanceData>(ConstructInstance("Sphere", "EmmissiveWhite", e.id()))
            .set<Transform>(t)
            .set<LightData>(lightData)
            .set<ObjectName>({ "Light" });



        m_geometry_pass = _world.query_builder<InstanceData, Transform>().build();
        m_light_pass = _world.query_builder<LightData, Transform>().build();

        m_render_targets.push_back(IGraphics::Get().CreateRenderTarget());
        m_depth_targets.push_back(IGraphics::Get().CreateDepthTarget());

        m_render_targets.push_back(IGraphics::Get().CreateRenderTarget(1));
        m_readback_buffers.push_back(IGraphics::Get().CreateReadBackBuffer(m_render_targets[1]));
        m_render_targets[1]->BindReadBackBuffer(m_readback_buffers[0]);
        e = IEngine::Get().CreateEntity("Scene Camera");
        e.add<CameraData>();
			
        //set this to take camera component. - default values for main view on render target. 
        m_camera.ChangeCamera(e.id());
        m_camera.SetPerspective(45, 1920.0f / 1080.0f, 0.1f, 1000000);
        m_camera.LookAt({ 0, 5, 10 }, { 0, 0, 0 }, { 0, 1, 0 });
        m_camera.SetDefaultID(e.id());
    }
#undef min
#undef max
    void GraphicsSys::PreUpdate(flecs::iter const& _info)
    {
        ImGui::Begin("View", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar);
        ImGui::BeginMenuBar();
        if (ImGui::MenuItem(ICON_FA_UP_DOWN_LEFT_RIGHT))
        {
            m_gizmo = GizmoMode::Translate;
        }
        if (ImGui::MenuItem(ICON_FA_ROTATE))
        {
            m_gizmo = GizmoMode::Rotate;
        }
        if (ImGui::MenuItem(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER))
        {
            m_gizmo = GizmoMode::Scale;
        }
        if (ImGuiMenuItemWithAlign(ICON_FA_PLAY, 0.5))
        {
            IEngine::Get().SetGameRunning(true);
        }
        ImGui::SameLine();
        if (ImGui::MenuItem(ICON_FA_PAUSE))
        {
            IEngine::Get().SetGameRunning(false);

        }
        if (Input::IsTriggered(Key::Space) && ImGui::IsWindowFocused())
        {
            IEngine::Get().SetGameRunning(!IEngine::Get().IsGameRunning());
        }

        ImGui::EndMenuBar();


        D3D12_VIEWPORT vp = std::static_pointer_cast<RenderTarget>(m_render_targets[0])->GetViewport();
        //ImVec2 stuff = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
        //if (vp.Width != stuff.x || vp.Height != stuff.y)
        //{
        //    m_render_targets[0]->SetDimensions(Vector2{ stuff.x, stuff.y });
        //    vp = std::static_pointer_cast<RenderTarget>(m_render_targets[0])->GetViewport();
        //}

        ImVec2 screen_center = (ImGui::GetWindowContentRegionMax() + ImGui::GetWindowContentRegionMin()) / 2.0f;
        ImVec2 size = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
        ImVec2 cursor_pos = { screen_center.x - (vp.Width / 2.0f), screen_center.y - (vp.Height / 2.0f) };

        ImGui::SetCursorPos(cursor_pos);


        ImGui::Image(
            (ImTextureID)m_render_targets[0]->GetPtr(),
            { 1920, 1080 },
            { 0, 0 },//{ std::max(-cursor_pos.x, vp.TopLeftX) / 1920.0f, std::max(-cursor_pos.y, vp.TopLeftY) / 1080.0f },
            { 1, 1 }//std::min(size.x, vp.Width) / 1920.0f, std::min(size.y, vp.Height) / 1080.0f }
        );

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_is_view_clicked = true;
        }

        if (m_is_view_clicked)
        {
            ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

            if (drag_delta.x == 0 && drag_delta.y == 0)
            {
                m_curr_drag_delta = { drag_delta.x, drag_delta.y };
            }

            float x = drag_delta.x - m_curr_drag_delta.x;
            float y = drag_delta.y - m_curr_drag_delta.y;
            m_camera.Pitch(y * _info.delta_time() * 5);
            m_curr_drag_delta = { drag_delta.x, drag_delta.y };
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

            if (Input::IsPressed(Mouse::Right))
            {
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
                if (Input::IsPressed(Key::R))
                    m_camera.ChangeCamera(m_camera.GetDefaultID());
            }


            m_camera.Update();

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                m_is_view_clicked = false;
            }

        }
        if (!Input::IsPressed(Mouse::Right))
        {
            if (Input::IsTriggered(Key::Q))
            {
                m_gizmo = GizmoMode::None;
            }
            if (Input::IsTriggered(Key::W))
            {
                m_gizmo = GizmoMode::Translate;
            }
            if (Input::IsTriggered(Key::E))
            {
                m_gizmo = GizmoMode::Rotate;
            }
            if (Input::IsTriggered(Key::R))
            {
                m_gizmo = GizmoMode::Scale;
            }
        }

        int objId = IEngine::Get().GetGUI().GetSelectedObject();
        if (objId != -1)
        {
            flecs::entity& current = IEngine::Get().GetWorld().entity(objId);
            Transform& transform = *current.get_mut<Transform>();

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            //compute viewport for ImGuizmo

            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 max = ImGui::GetItemRectMax() - ImGui::GetItemRectMin();
            //min += pos;

            //shows a box of where the gizmo will be drawn on
            //ImGui::GetForegroundDrawList()->AddRect(min, min + imageSize, ImColor(0, 255, 0));

            ImGuizmo::SetRect(min.x, min.y, max.x, max.y);
            SimpleMath::Matrix matrix = transform.matrix;
            switch (m_gizmo)
            {
            case GizmoMode::None:
            {

                break;
            }
            case GizmoMode::Translate:
            {
                ImGuizmo::Manipulate(&(m_camera.View().m[0][0]), &(m_camera.Projection().m[0][0]), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, &matrix.m[0][0]);
                break;
            }
            case GizmoMode::Rotate:
            {
                ImGuizmo::Manipulate(&(m_camera.View().m[0][0]), &(m_camera.Projection().m[0][0]), ImGuizmo::ROTATE, ImGuizmo::WORLD, &matrix.m[0][0]);
                break;
            }
            case GizmoMode::Scale:
            {
                ImGuizmo::Manipulate(&(m_camera.View().m[0][0]), &(m_camera.Projection().m[0][0]), ImGuizmo::SCALE, ImGuizmo::WORLD, &matrix.m[0][0]);
                break;
            }

            }

            if (ImGuizmo::IsUsingAny()) //compute only when we modify 
            {
                Vector3 euler;
                ImGuizmo::DecomposeMatrixToComponents(&matrix.m[0][0], &transform.position.x, &euler.x, &transform.scale.x);
                matrix.Decompose(transform.scale, transform.orientation, transform.position);
            }
        }

        if (ImGui::IsItemClicked() && !ImGuizmo::IsUsingAny())
        {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();

            //if (mouse_pos.x < (max.x) && mouse_pos.y < (max.y)
            //    && mouse_pos.x >= min.x && mouse_pos.y >= min.y)
            {
                Vector2 pos = {
                    mouse_pos.x - min.x,
                    mouse_pos.y - min.y
                };
                UINT8* pdata = nullptr;
                if (m_readback_buffers[0]->Map(&pdata))
                {
                    float* pfloat = reinterpret_cast<float*>(pdata);
                    size_t p = (size_t)(pos.y + vp.TopLeftY) * (size_t)(vp.TopLeftX + vp.Width) + (size_t)(vp.TopLeftX + pos.x);
                    float id = pfloat[p];
                    if (objId != -1)
                        IEngine::Get().GetWorld().entity(objId).get_mut<InstanceData>()->m_stencil = 0;
                    if ((int)id != 0)
                    {
                        IEngine::Get().GetGUI().SetSelectedObject((int)id);
                        IEngine::Get().GetWorld().entity((int)id).get_mut<InstanceData>()->m_stencil = 1;
                        if (m_gizmo == None)
                        {
                            m_gizmo = Translate;
                        }
                    }
                }
            }
        }

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


        ImGui::End();
        //ImGui::PopStyleVar();
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
        m_render_targets[0]->SetCameraPosition(m_camera.GetPosition());
        m_render_targets[0]->SetView(m_camera.View());
        m_render_targets[0]->SetProjection(m_camera.Projection());

        IGraphics& graphics = IGraphics::Get();
        m_geometry_pass.each(
            [&graphics](InstanceData const& _instance, Transform const& _transform)
            {
                if (_instance.m_material != nullptr)
                    graphics.Draw(DrawCall{ _transform.matrix, _instance });
            });

        m_light_pass.each(
            [&graphics](LightData const& _light, Transform const& _transform)
            {
                graphics.SetLight(_light.id, true);
                graphics.SetLight(_light.id, _transform.position, _light.color);
            });
    }

    void GraphicsSys::OnUpdate(InstanceData const& _instance, Transform const& _transform) const
    {
        //IGraphics::Get().UpdateInstance(_instance.id, _transform.matrix);
    }

    void GraphicsSys::PostUpdate(flecs::iter const& _info)
    {

    }

    void GraphicsSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
    {
        using namespace nlohmann;
        if (type == "InstanceData")
        {
            const InstanceData* data = _entity.get<InstanceData>();
            json obj = json::object();
            obj["Type"] = "InstanceData";
            obj["Mesh"] = data->m_vertex_buffer->Name();
            obj["Material"] = data->m_material->Path();
            _components.push_back(obj);
        }
        else if (type == "LightData")
        {
            const LightData* data = _entity.get<LightData>();
            json obj = json::object();
            obj["Type"] = "LightData";
            obj["Color"] = WriteVec3(data->color);
            _components.push_back(obj);
        }
    }

    void GraphicsSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& _type)
    {
        using namespace nlohmann;
        if (_type == "InstanceData")
        {
            _object.set<InstanceData>(ConstructInstance(_data["Mesh"], _data["Material"], _object.id()));
        }
        else if (_type == "LightData")
        {
            _object.set<LightData>({ ReadVec3(_data["Color"]), light_id++ });
        }
    }

    void GraphicsSys::GuiDisplay(flecs::entity& _entity, const std::string& _type)
    {
        if (_type == "InstanceData")
        {
            if (_entity.has<InstanceData>())
            {
                InstanceData* data = _entity.get_mut<InstanceData>();

                if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::BeginCombo("###mesh", data->m_vertex_buffer->Name().c_str()))
                    {
                        for (const auto& [name, id] : m_mesh_map)
                        {
                            bool selected = (id == data->m_vertex_buffer);
                            if (ImGui::Selectable(name.c_str(), &selected))
                            {
                                data->m_vertex_buffer = id;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::BeginCombo("###material", data->m_material->Name().c_str()))
                    {
                        for (const auto& [name, id] : m_material_map)
                        {
                            bool selected = (id == data->m_material);
                            if (ImGui::Selectable(name.c_str(), &selected))
                            {
                                data->m_material = id;
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Edit"))
                        m_material_edit = true;

                    if (m_material_edit)
                    {
                        ImGui::Begin("Material Edit", &m_material_edit);

                        data->m_material->RenderImGui();

                        ImGui::End();
                    }
                    ImGui::TreePop();
                }

            }
        }

        else if (_type == "LightData")
        {
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
}
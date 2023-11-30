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

		for (auto it = _animation.nodes.begin();
			it != _animation.nodes.end(); ++it)
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
        REGISTER_TO_SERIALIZER(Renderer, this);
        REGISTER_TO_SERIALIZER(LightData, this);
        REGISTER_TO_SERIALIZER(Material, this);
        REGISTER_TO_DESERIALIZER(Renderer, this);
        REGISTER_TO_DESERIALIZER(LightData, this);
        REGISTER_TO_DESERIALIZER(Material, this);
		
        IEngine::Get().GetGUI().RegisterComponent("Renderer",
            std::bind(&GraphicsSys::GuiDisplay,
                this, std::placeholders::_1, std::placeholders::_2),
            [this](flecs::entity& _entity)
            {
                Renderer renderer{};
                renderer.m_material = ResourceLoader::Get()
                    .GetOrLoadResource<MaterialImpl>("Assets/materials/Default.mat");
                renderer.m_vertex_buffer = ResourceLoader::Get()
                    .GetOrLoadResource<VertexBuffer>("Cube");
                renderer.m_id = _entity.id();
                _entity.set<Renderer>(renderer);
                _entity.set<Material>({ renderer.m_material });
            });

		IEngine::Get().GetGUI().RegisterComponent(
			"LightData",
			std::bind(&GraphicsSys::GuiDisplay,
				this,
				std::placeholders::_1,
				std::placeholders::_2
			),
			[this](flecs::entity& _entity)
			{
				_entity.set<LightData>({ {1, 1, 1} });
			});

		// Meshes
		IGraphics& graphics = IGraphics::Get();
		ResourceLoader& loader = ResourceLoader::Get();
		loader.GetOrLoadResource<VertexBuffer>("Cube");
		loader.GetOrLoadResource<VertexBuffer>("Sphere");

		_world.system("Editor PreRender")
			.kind(flecs::PreUpdate)
			.kind<Editor>()
			.iter([this](flecs::iter const& _info) { PreUpdate(_info); });

		_world.system("Editor Render")
			.kind(flecs::OnUpdate)
			.kind<Editor>()
			.iter([this](flecs::iter const& _info) { OnUpdate(_info); });

		_world.system("Editor PostRender")
			.kind(flecs::PostUpdate)
			.kind<Editor>()
			.iter([this](flecs::iter const& _info) { PostUpdate(_info); });

		_world.system("PreRender")
			.kind(flecs::PreUpdate)
			.iter([this](flecs::iter const& _info) { PreUpdate(_info); });

		_world.system("Render")
			.kind(flecs::OnUpdate)
			.iter([this](flecs::iter const& _info) { OnUpdate(_info); });
		_world.system("PostRender")
			.kind(flecs::PostUpdate)
			.iter([this](flecs::iter const& _info) { PostUpdate(_info); });


		loader.GetOrLoadResource<Pipeline>("Assets/Pipelines/Default.json");
		loader.GetOrLoadResource<Pipeline>("Assets/Pipelines/Skybox.json");

        loader.GetOrLoadResource<MaterialImpl>("Assets/materials/Default.mat");
        loader.GetOrLoadResource<MaterialImpl>("Assets/materials/EmissiveWhite.mat");
        loader.GetOrLoadResource<MaterialImpl>("Assets/materials/EmissiveRed.mat");
        loader.GetOrLoadResource<MaterialImpl>("Assets/materials/Skybox.mat");



		m_geometry_pass = _world.query_builder<Renderer, Transform>().build();
		m_light_pass = _world.query_builder<LightData, Transform>().build();

		m_render_targets.push_back(IGraphics::Get().CreateRenderTarget());
		m_depth_targets.push_back(IGraphics::Get().CreateDepthTarget());

		m_render_targets.push_back(IGraphics::Get().CreateRenderTarget(1));
		m_readback_buffers.push_back(
			IGraphics::Get().CreateReadBackBuffer(m_render_targets[1]));
		m_render_targets[1]->BindReadBackBuffer(m_readback_buffers[0]);
		auto e = _world.entity("Scene camera");
		e.add<SceneCamera>()
			.set<ObjectName>({ "Scene Camera" });

        //AM ACTIVELY setting scene camera back as i dont want it to be in the editor. 
  
			
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
		ImGui::Begin("View", (bool*)0,
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_MenuBar
		);

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
		IEngine& engine = IEngine::Get();
		ISceneManager& scene_manager = ISceneManager::Get();
		if (!m_running)
		{
			if (ImGuiMenuItemWithAlign(ICON_FA_PLAY, 0.5))
			{
				scene_manager.SaveAllScenes();
				engine.SetGameRunning(true);
				m_running = true;
			}

		}
		else
		{
			if (ImGuiMenuItemWithAlign(ICON_FA_STOP, 0.5))
			{
				engine.GetWorld().defer([]() { ISceneManager::Get().ReloadScenes(); });
				engine.SetGameRunning(false);
				m_running = false;
			}
			if (engine.IsGameRunning())
			{
				ImGui::SameLine();
				if (ImGui::MenuItem(ICON_FA_PAUSE))
				{
					engine.SetGameRunning(false);

				}

			}
			else
			{
				ImGui::SameLine();
				if (ImGui::MenuItem(ICON_FA_PLAY))
				{
					engine.SetGameRunning(true);

				}
			}
		}

		//if (Input::IsTriggered(Key::Space) && ImGui::IsWindowFocused())
		//{
		//	IEngine::Get().SetGameRunning(!IEngine::Get().IsGameRunning());
		//}

		ImGui::EndMenuBar();


		D3D12_VIEWPORT vp = std::static_pointer_cast<RenderTarget>(
			m_render_targets[0])->GetViewport();

		ImVec2 screen_center = (
			ImGui::GetWindowContentRegionMax() +
			ImGui::GetWindowContentRegionMin()) / 2.0f;
		ImVec2 size = ImGui::GetWindowContentRegionMax() -
			ImGui::GetWindowContentRegionMin();
		ImVec2 cursor_pos = {
			screen_center.x - (vp.Width / 2.0f),
			screen_center.y - (vp.Height / 2.0f) };

		ImGui::SetCursorPos(cursor_pos);
		ImGui::Image(
			(ImTextureID)m_render_targets[0]->GetPtr(),
			{ 1920, 1080 },
			{ 0, 0 },
			{ 1, 1 }
		);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			m_is_view_clicked = true;
		}
#pragma region running
		if (!IEngine::Get().IsGameRunning())
		{

        float speed = 5;
        if (ImGui::GetIO().MouseWheel > 0 && ImGui::IsWindowFocused())
            m_camera.MoveForward(_info.delta_time() * speed);
        if (ImGui::GetIO().MouseWheel < 0 && ImGui::IsWindowFocused())
            m_camera.MoveForward(_info.delta_time() * -speed);
        m_camera.Update();

		if (m_is_view_clicked)
		{
			ImVec2 drag_delta
				= ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

			if (drag_delta.x == 0 && drag_delta.y == 0)
			{
				m_curr_drag_delta = { drag_delta.x, drag_delta.y };
			}

			float x = drag_delta.x - m_curr_drag_delta.x;
			float y = drag_delta.y - m_curr_drag_delta.y;
			//changed to 25 so release worked way better we should add a slider for sensitivity
			m_camera.Pitch(y * _info.delta_time() * 25);
			m_curr_drag_delta = { drag_delta.x, drag_delta.y };
			m_camera.Yaw(x * _info.delta_time() * -25);
			
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
		}

        int objId = IEngine::Get().GetGUI().GetSelectedObject();
        if (objId != -1)
        {
            flecs::entity& current = IEngine::Get().GetWorld().entity(objId);
            Transform worldTransform = TransformSys::GetWorldTransform(current);

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			//compute viewport for ImGuizmo

			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 pos = ImGui::GetWindowPos();
			ImVec2 max = ImGui::GetItemRectMax() - ImGui::GetItemRectMin();

            ImGuizmo::SetRect(min.x, min.y, max.x, max.y);
            SimpleMath::Matrix matrix = worldTransform.matrix;

            switch (m_gizmo)
            {
            case GizmoMode::None:
            {

                break;
            }
            case GizmoMode::Translate:
            {
                ImGuizmo::Manipulate(
                    &(m_camera.View().m[0][0]),
                    &(m_camera.Projection().m[0][0]),
                    ImGuizmo::TRANSLATE,
                    ImGuizmo::LOCAL,
                    &matrix.m[0][0]
                );
                break;
            }
            case GizmoMode::Rotate:
            {
                ImGuizmo::Manipulate(
                    &(m_camera.View().m[0][0]),
                    &(m_camera.Projection().m[0][0]),
                    ImGuizmo::ROTATE,
                    ImGuizmo::LOCAL,
                    &matrix.m[0][0]
                );
                break;
            }
            case GizmoMode::Scale:
            {
                ImGuizmo::Manipulate(
                    &(m_camera.View().m[0][0]),
                    &(m_camera.Projection().m[0][0]),
                    ImGuizmo::SCALE,
                    ImGuizmo::LOCAL,
                    &matrix.m[0][0]
                );
                break;
            }

            }

            if (ImGuizmo::IsUsingAny()) //compute only when we modify 
            {
                Vector3 euler;
                ImGuizmo::DecomposeMatrixToComponents(
                    &matrix.m[0][0],
                    &worldTransform.position.x,
                    &euler.x,
                    &worldTransform.scale.x
                );
                matrix.Decompose(
                    worldTransform.scale,
                    worldTransform.orientation,
                    worldTransform.position
                );

                // update the original transform of the entity
                Transform& originalTransform = *current.get_mut<Transform>();
                if (current.parent().is_valid() && current.parent().has<IsScene>()==false)
                {
                    Transform parentTransform = TransformSys::GetWorldTransform(current.parent());
                    Matrix worldToParent = parentTransform.matrix.Invert();
                    originalTransform.position= Vector3::Transform(worldTransform.position, worldToParent);                    

					Quaternion parentInverseOrientation;
					parentTransform.orientation.Inverse(parentInverseOrientation);
                    originalTransform.orientation = worldTransform.orientation* parentInverseOrientation; //the multiplication of quaternions is done from left to right

                    Vector3 parentScale = parentTransform.scale;
                    originalTransform.scale = { worldTransform.scale.x / parentScale.x, worldTransform.scale.y / parentScale.y, worldTransform.scale.z / parentScale.z };
                }
                else 
                {
                    originalTransform.position = worldTransform.position;
                    originalTransform.orientation = worldTransform.orientation;
                    originalTransform.scale = worldTransform.scale;
                }

            }
        }

		if (ImGui::IsItemClicked() && !ImGuizmo::IsUsingAny())
		{
			ImVec2 mouse_pos = ImGui::GetMousePos();
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();

			Vector2 pos = {
				mouse_pos.x - min.x,
				mouse_pos.y - min.y
			};
			UINT8* pdata = nullptr;
			if (m_readback_buffers[0]->Map(&pdata))
			{
				float* pfloat = reinterpret_cast<float*>(pdata);
				size_t p =
					(size_t)(pos.y + vp.TopLeftY) *
					(size_t)(vp.TopLeftX + vp.Width) +
					(size_t)(vp.TopLeftX + pos.x);
				float id = pfloat[p];
				if (objId != -1)
				{
					auto e = IEngine::Get().GetWorld().entity(objId);
					if (e.has<Renderer>())
						e.get_mut<Renderer>()->m_stencil = 0;
				}
				if ((int)id != 0)
				{
					IEngine::Get().GetGUI().SetSelectedObject((int)id);

					auto e = IEngine::Get().GetWorld().entity((int)id);
					if (e.has<Renderer>())
						e.get_mut<Renderer>()->m_stencil = 1;
					if (m_gizmo == None)
					{
						m_gizmo = Translate;
					}
				}
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload =
				ImGui::AcceptDragDropPayload(
					"PREFAB", ImGuiDragDropFlags_None))
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
#pragma endregion playing
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
		IGraphics& graphics = IGraphics::Get();
		graphics.SetCamera(
			m_camera.GetPosition(),
			m_camera.View() * m_camera.Projection()
		);
		m_geometry_pass.each(
			[&graphics](Renderer const& _instance, Transform const& _transform)
			{
				if (_instance.m_material != nullptr)
					graphics.Draw(DrawCall{ _transform.matrix, _instance });
			});

		m_light_pass.each(
			[&graphics](LightData const& _light, Transform const& _transform)
			{
				graphics.AddLight({ _transform.position, _light.color });
			});
	}

	void GraphicsSys::OnUpdate(
		Renderer const& _instance,
		Transform const& _transform
	) const
	{
		//IGraphics::Get().UpdateInstance(_instance.id, _transform.matrix);
	}

	void GraphicsSys::PostUpdate(flecs::iter const& _info)
	{

	}

	void GraphicsSys::Write(const flecs::entity& _entity,
		std::vector<nlohmann::json>& _components, const std::string& type)
	{
		using namespace nlohmann;
		if (type == "Renderer")
		{
			const Renderer* data = _entity.get<Renderer>();
			json obj = json::object();
			obj["Type"] = "Renderer";
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

    void GraphicsSys::Read(flecs::entity& _object,
        nlohmann::json& _data, const std::string& _type)
    {
        using namespace nlohmann;
        if (_type == "Renderer")
        {
            Renderer renderer{};
            renderer.m_id = _object.id();
            renderer.m_material = ResourceLoader::Get()
                .GetOrLoadResource<MaterialImpl>(_data["Material"].get<std::string>());
            renderer.m_vertex_buffer = ResourceLoader::Get()
                .GetOrLoadResource<VertexBuffer>(_data["Mesh"].get<std::string>());
            _object.set<Renderer>(renderer);
            _object.set<Material>(Material{ renderer.m_material });
        }
        else if (_type == "LightData")
        {
            _object.set<LightData>({ ReadVec3(_data["Color"]) });
        }
    }

	void GraphicsSys::GuiDisplay(flecs::entity& _entity,
		const std::string& _type)
	{
		if (_type == "Renderer")
		{
			if (_entity.has<Renderer>())
			{
				Renderer* data = _entity.get_mut<Renderer>();
				bool is_open = ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup("Renderer Popup");
				}
				if (ImGui::BeginPopup("Renderer Popup"))
				{
					if (ImGui::Button("Remove Component"))
					{
						_entity.remove<Renderer>();
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (is_open)
				{
					ImGui::Text("Mesh");
					if (ImGui::BeginCombo("###mesh",
						data->m_vertex_buffer->Name().c_str()))
					{
						auto& mesh_map = ResourceLoader::Get().GetResourceMap<VertexBuffer>();
						for (const auto& [name, mesh] : mesh_map)
						{
							bool selected = (mesh == data->m_vertex_buffer);
							if (ImGui::Selectable(name.c_str(), &selected))
							{
								data->m_vertex_buffer = std::dynamic_pointer_cast<VertexBuffer>(mesh);
							}
						}
						ImGui::EndCombo();
					}
					ImGui::Text("Material");
					if (ImGui::BeginCombo(
						"###material", data->m_material->Name().c_str()))
					{
						auto& material_map = ResourceLoader::Get().GetResourceMap<MaterialImpl>();
						for (const auto& [name, material] : material_map)
						{
							bool selected = (material == data->m_material);
							if (ImGui::Selectable(name.c_str(), &selected))
							{
								data->m_material = std::dynamic_pointer_cast<MaterialImpl>(material);
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
					ImGui::Text("");
				}
			}
		}

		else if (_type == "LightData")
		{
			if (_entity.has<LightData>())
			{
				LightData* data = _entity.get_mut<LightData>();
				bool is_open = ImGui::CollapsingHeader("Light",
					ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup("Light Popup");
				}
				if (ImGui::BeginPopup("Light Popup"))
				{
					if (ImGui::Button("Remove Component"))
					{
						_entity.remove<Renderer>();
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				if (is_open)
				{
					ImGui::ColorPicker3("Color", &data->color.x);
				}
				ImGui::Text("");
			}
		}
	}
}
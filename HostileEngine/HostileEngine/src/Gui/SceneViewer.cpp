//------------------------------------------------------------------------------
//
// File Name:	SceneViewer.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "SceneViewer.h"
#include "imgui.h"
#include "Engine.h"
#include "flecs.h"
#include "TransformSys.h"
#include <iostream>
#include "TransformSys.h"
#include "misc/cpp/imgui_stdlib.h"
#include "imgui_internal.h"
#include "Input.h"


namespace Hostile
{
	flecs::entity* SceneViewer::m_to_delete = nullptr;
	void SceneViewer::Render(DisplayMap& _display, AddMap& _add)
	{
		if (Input::IsPressed(Key::LeftControl) && Input::IsTriggered(Key::S))
		{
			IEngine::Get().GetCurrentScene()->Save();
		}

		int selected_obj = -1;
		flecs::world& world = IEngine::Get().GetWorld();
		ImGui::Begin("Scene Veiwer");
		if (ImGui::Button("Make Blank Entity"))
		{
			world.defer([&]() {
				IEngine& engine = IEngine::Get();
				flecs::entity entity = engine.CreateEntity();
				selected_obj = entity.id();
				if (engine.GetCurrentScene())
				{
					engine.GetCurrentScene()->Add(entity);
				}
				else
				{
					engine.AddScene("Basic Scene").Add(entity);
					engine.SetCurrentScene("Basic Scene");
				}
				});
		}
		flecs::query<IsScene> q = world.query<IsScene>();
		world.defer([&]() {
			q.each([&](flecs::entity _e, IsScene& _T)
				{
					if (IEngine::Get().GetCurrentScene() == nullptr)
					{
						IEngine::Get().SetCurrentScene(_e.get<ObjectName>()->name);
					}
					DisplayScene(_e, &m_selected);
				});
			});


		ImGui::End();

		m_inspector.Render(m_selected, _display, _add);

	}

	void SceneViewer::DisplayScene(flecs::entity _entity, int* _id)
	{
		IEngine& engine = IEngine::Get();
		std::string popup_name = "Scene: ";
		popup_name += std::to_string(_entity.id());
		
		bool node_open = ImGui::CollapsingHeader(_entity.get_ref<ObjectName>()->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup(popup_name.c_str());
		}
		if (node_open)
		{
			_entity.children([&](flecs::entity target) {DisplayEntity(target, _id); });
		}

		if (ImGui::IsItemClicked())
		{
			engine.SetCurrentScene(_entity.get_ref<ObjectName>()->name);
		}

		if (ImGui::BeginPopup(popup_name.c_str()))
		{
			if (ImGui::Button("Unload"))
			{
				m_selected = -1;
				engine.UnloadScene(_entity.id());
			}
			ImGui::EndPopup();
		}

	}

	void SceneViewer::SetSelectedObject(int _id)
	{
		m_selected = _id;
	}

	void SceneViewer::DisplayEntity(flecs::entity _entity, int* _id)
	{

		ImGuiTreeNodeFlags leaf_flags = 0;
		leaf_flags |= ImGuiTreeNodeFlags_Leaf;
		bool has_child = false;
		int counter = 0;
		_entity.children([&](flecs::entity target) { has_child = true; });
		if (!has_child)
		{
			std::string name;
			if (_entity.has<ObjectName>())
			{
				name = _entity.get_ref<ObjectName>()->name;

			}
			else
			{
				name = "ERROR";
			}
			ImGui::TreeNodeEx(name.c_str(), leaf_flags);

			if (ImGui::IsItemClicked())
			{
				*_id = _entity.id();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup((std::to_string(_entity.id()) + "Child").c_str());
				m_to_delete = &_entity;
			}
			ImGui::SetNextWindowSize({ 300,150 });
			if (ImGui::BeginPopup((std::to_string(_entity.id()) + "Child").c_str()))
			{

				if (ImGui::Button("Delete"))
				{
					if (*_id == m_to_delete->id())
					{
						*_id = -1;
					}
					IEngine::Get().GetWorld().defer([&]() {m_to_delete->destruct(); });
				}
				ImGui::EndPopup();
			}

			DragAndDrop(_entity);
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::TreeNode(_entity.get_ref<ObjectName>()->name.c_str()))
			{
				if (ImGui::IsItemClicked())
				{
					*_id = _entity.id();
				}
				//_display = &_entity;
				DragAndDrop(_entity);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup((std::to_string(_entity.id()) + "Parent").c_str());
					m_to_delete = &_entity;
				}
				ImGui::SetNextWindowSize({ 300,150 });
				if (ImGui::BeginPopup((std::to_string(_entity.id()) + "Parent").c_str()))
				{

					if (ImGui::Button("Delete"))
					{
						if (*_id == m_to_delete->id())
						{
							*_id = -1;
						}
						IEngine::Get().GetWorld().defer([&]() {m_to_delete->destruct(); });
					}
					ImGui::EndPopup();
				}

				_entity.children([&](flecs::entity target) { DisplayEntity(target, _id); });
				ImGui::TreePop();
			}

		}

	}
	void SceneViewer::DragAndDrop(flecs::entity _entity)
	{
		ImGuiDragDropFlags target_flags = 0;
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("_TREENODE", &_entity, sizeof(_entity));
			std::string name = _entity.get<ObjectName>()->name;
			ImGui::Text(name.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE", target_flags))
			{
				flecs::entity entity = *static_cast<flecs::entity*>(payload->Data);
				if (entity.has(flecs::Wildcard, flecs::ChildOf))
				{
					entity.children([](flecs::entity target) {});
				}
				bool is_ok = true;
				_entity.children([&](flecs::entity target) {if (target.name() == entity.name())is_ok = false; });
				if (is_ok)
				{
					//changed the parent-child relationship assignment to preserve the child's current position and scale, 
					//ensuring spatial properties remain unchanged upon establishing hierarchy
					Transform* childTransform = entity.get_mut<Transform>();
					Transform prtTransform = TransformSys::GetWorldTransform(_entity);
					childTransform->position = Vector3::Transform(childTransform->position, prtTransform.matrix.Invert());
					childTransform->scale *= {1.f / prtTransform.scale.x, 1.f / prtTransform.scale.y, 1.f / prtTransform.scale.z};

					entity.child_of(_entity);
				}
				else
				{
					Log::Error("Actor with that name already exists. Change name if you want to parent this actor");
				}


			}
			ImGui::EndDragDropTarget();
		}
	}
	void SceneViewer::DragAndDropRoot()
	{
		ImGuiDragDropFlags target_flags = 0;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_TREENODE", target_flags))
			{
				flecs::entity entity = *static_cast<flecs::entity*>(payload->Data);
				entity.remove(flecs::ChildOf, entity.parent());
			}
			ImGui::EndDragDropTarget();
		}
	}
}
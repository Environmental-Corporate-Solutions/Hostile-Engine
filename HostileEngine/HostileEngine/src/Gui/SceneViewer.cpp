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

namespace Hostile
{
	flecs::entity* SceneViewer::m_to_delete = nullptr;
	void SceneViewer::Render(DisplayMap& _display, AddMap& _add)
	{
		int selected_obj = -1;
		flecs::world& world = IEngine::Get().GetWorld();
		ImGui::Begin("Scen Veiwer");
		if (ImGui::Button("Make Blank Entity"))
		{
			flecs::entity& entity = IEngine::Get().CreateEntity();
			selected_obj = entity.id();
		}
		flecs::query<Transform> q = world.query<Transform>();
		world.defer([&]() {
			ImGuiTreeNodeFlags node_flag = 0;
			node_flag |= ImGuiTreeNodeFlags_DefaultOpen;
			if (ImGui::TreeNodeEx("Scene", node_flag))
			{
				DragAndDropRoot();
				q.each([&](flecs::entity _e, Transform& _T)
					{
						if (!_e.parent().is_valid())
						{
							DisplayEntity(_e, &m_selected);
						}
					});
				ImGui::TreePop();
			}
			});


		ImGui::End();

		m_inspector.Render(m_selected, _display, _add);

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
				ImGui::OpenPopup("###Object Options Child");
				m_to_delete = &_entity;
			}
			ImGui::SetNextWindowSize({ 300,150 });
			if (ImGui::BeginPopup("###Object Options Child"))
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
					ImGui::OpenPopup("###Object Options Parent");
					m_to_delete = &_entity;
				}
				ImGui::SetNextWindowSize({ 300,150 });
				if (ImGui::BeginPopup("###Object Options Parent"))
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
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

namespace Hostile
{
  static std::vector<const char*> names;
  void SceneViewer::Render()
  {
    flecs::world& world = IEngine::Get().GetWorld();
    ImGui::Begin("Scen Veiwer");
    if (ImGui::Button("Make Blank Entity"))
    {
      flecs::entity entity = world.entity();
      std::string name = "Actor";
      name += std::to_string(counter++);
      entity.set_name(name.c_str());
      entity.add<Transform>();


      //flecs::entity baby3 = world.entity();
      //baby3.set_name("baby3");
      //baby3.child_of(entity);
      //flecs::entity baby2 = world.entity();
      //baby2.set_name("baby2");
      //baby2.child_of(baby);
    }
    flecs::query<Transform> q = world.query<Transform>();
    world.defer([&]() {
      ImGuiTreeNodeFlags leaf_flags = 0;
      leaf_flags |= ImGuiTreeNodeFlags_DefaultOpen;
      if (ImGui::TreeNodeEx("Scene", leaf_flags))
      {
        DragAndDropRoot();
        q.each([&](flecs::entity _e, Transform& _T)
          {
            if (!_e.parent().is_valid())
            {
              DisplayEntity(_e);
            }
          });
        ImGui::TreePop();
      }
      });

    if (selected_obj != -1)
    {
      m_selected = selected_obj;
    }
    ImGui::End();
    ImGui::Begin("Inspector");
    if (m_selected != -1)
    {
      flecs::entity current = world.entity(m_selected);
      ImGui::Text(current.name().c_str());
      const Transform* transform = current.get<Transform>();
      Transform trans = *transform;
      ImGui::InputFloat3("Position", &trans.position.x);
      current.set<Transform>(trans);
      //call inspector view later
    }
    ImGui::End();
  }
  void SceneViewer::DisplayEntity(flecs::entity _entity)
  {
    ImGuiTreeNodeFlags leaf_flags;
    leaf_flags |= ImGuiTreeNodeFlags_Leaf;
    bool has_child = false;
    int counter = 0;
    _entity.children([&](flecs::entity target) {has_child = true; });
    if (!has_child)
    {
      std::string name = _entity.name();
      ImGui::TreeNodeEx(_entity.name().c_str(),leaf_flags);

      DragAndDrop(_entity);

    }
    else
    {
      if (ImGui::TreeNode(_entity.name().c_str()))
      {

        DragAndDrop(_entity);
        _entity.children([](flecs::entity target) {DisplayEntity(target); });
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
      std::string name = _entity.name().c_str();
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
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
      std::string name = "New gamer ";
      name += std::to_string(counter++);
      entity.set_name(name.c_str());
      entity.add<Transform>();
      flecs::entity baby = world.entity();
      baby.set_name("baby");
      baby.add<Transform>();
      flecs::entity baby3 = world.entity();
      baby3.set_name("baby3");
      baby3.child_of(entity);
      baby.child_of(entity);
      flecs::entity baby2 = world.entity();
      baby2.set_name("baby2");
      baby2.child_of(baby);
    }

    flecs::query<Transform> q = world.query<Transform>();
    
    q.each([](flecs::entity _e, Transform& _T)
      {
        if (!_e.parent().is_valid())
        {
          DisplayEntity(_e);
        }
      });



    ImGui::End();
  }
  void SceneViewer::DisplayEntity(flecs::entity _entity)
  {
    ImGuiTreeNodeFlags node_flags;
    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet
    bool has_child = false;
    _entity.children([&](flecs::entity target) {has_child = true; });
    if (!has_child)
    {
      int counter = 0;
      ImGui::TreeNodeEx((void*)(intptr_t)counter++, node_flags, _entity.name().c_str());
      IEngine::Get().GetWorld()

    }
    else
    {
      if (ImGui::TreeNode(_entity.name().c_str()))
      {
        _entity.children([](flecs::entity target) {DisplayEntity(target); });
        ImGui::TreePop();
      }
    }

  }
}
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

namespace Hostile
{
  static std::vector<const char *> names;
  void SceneViewer::Render()
  {
    names.clear();
    flecs::world& world = IEngine::Get().GetWorld();
    ImGui::Begin("Scen Veiwer");
    if (ImGui::Button("Make Blank Entity"))
    {
      flecs::entity entity = world.entity();
      std::string name = "New gamer ";
      name += std::to_string(counter++);
      entity.set_name(name.c_str());
      entity.add<Transform>();
    }
    
    auto q = world.query<Transform>();
    q.each([](flecs::entity _e, Transform& _T) 
      {
        //ImGui::Text(_e.name().c_str());
        names.push_back(_e.name().c_str());
      });
    ImGui::Combo("list", &m_currentobj, names.data(),names.size());
 
    
    //q.each()


    ImGui::End();
  }
}
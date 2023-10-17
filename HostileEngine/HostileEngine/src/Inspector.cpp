//------------------------------------------------------------------------------
//
// File Name:	Inspector.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Inspector.h"
#include "imgui.h"
#include "flecs.h"
#include "Engine.h"
#include "Input.h"
#include <fstream>


//components
#include "TransformSys.h"
#include "Graphics/GraphicsSystem.h"
namespace Hostile
{
  void Inspector::Render(int _id, std::unordered_map<std::string, ISystemPtr>& _map)
  {
    ImGui::Begin("Inspector ###inspector");
    if (_id != -1)
    {
      flecs::world& world = IEngine::Get().GetWorld();
      flecs::entity current = world.entity(_id);

      if (ImGui::Button("Save to file"))
      {
        ISerializer::Get().WriteEntity(current);
      }
       
      current.each([&](flecs::id _id) {
        if (!_id.is_pair())
        {
          if (_map.find(_id.entity().name().c_str()) != _map.end())
          {
            ISystemPtr sys = _map[_id.entity().name().c_str()];
            sys->GuiDisplay(current);
          }
        }
        });

    }
    ImGui::End();
  }
}
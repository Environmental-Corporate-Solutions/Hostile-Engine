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
#include <numbers>
#include "Input.h"
#include <fstream>


//components
#include "TransformSys.h"
#include "GraphicsSystem.h"
namespace Hostile
{
  void Inspector::Render(int _id)
  {
    ImGui::Begin("Inspector ###inspector");
    if (_id != -1)
    {
      flecs::world& world = IEngine::Get().GetWorld();
      flecs::entity current = world.entity(_id);

      if (Input::IsTriggered(KeyCode::RightBracket))
      {
        auto e = world.entity();
        e.set_name("To File");
        e.add<Transform>();
        e.add<Mesh>();

        IEngine& engine = IEngine::Get();
        engine.GetSerializer().WriteEntity(e,engine.GetMap());

      }




      if (current.has<Transform>())
      {
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
          const Transform* transform = current.get<Transform>();
          Transform trans = *transform;
          ImGui::DragFloat3("Position", &trans.position.x);
          Vector3 rot = trans.orientation.ToEuler();
          rot *= 180.0f / PI;
          ImGui::DragFloat3("Rotation", &rot.x);
          rot *= PI / 180.0f;
          trans.orientation = Quaternion::CreateFromYawPitchRoll(rot);
          ImGui::DragFloat3("Scale", &trans.scale.x);


          current.set<Transform>(trans);
          ImGui::TreePop();
        }
        if (current.has<Mesh>())
        {
          if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
          {
            const Mesh* mesh = current.get<Mesh>();
            ImGui::Text(mesh->meshName.c_str());
            ImGui::TreePop();
          }
          current.each([](flecs::id _id) {
            if (!_id.is_pair())
            {
              ImGui::Text(_id.entity().name().c_str());
            }
            }); 
        }
      }
      //call inspector view later



    }
    ImGui::End();
  }
}
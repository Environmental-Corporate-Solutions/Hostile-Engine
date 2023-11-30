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
#include "misc/cpp/imgui_stdlib.h"


//components
#include "TransformSys.h"
#include "Graphics/GraphicsSystem.h"
namespace Hostile
{
	void Inspector::Render(int _id, DisplayMap& _display, AddMap& _add)
	{
		ImGui::Begin("Inspector ###inspector");
		if (_id != -1)
		{

			flecs::world& world = IEngine::Get().GetWorld();
			flecs::entity current = world.entity(_id);

			if (ImGui::Button("Save to file"))
			{
				ISerializer::Get().WriteEntityToFile(current);
			}
			if (current.is_valid())
			{
				m_obj_name = current.get<ObjectName>()->name;
				ImGui::InputText("Name", &m_obj_name);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					current.set<ObjectName>({ m_obj_name });
				}
				current.each([&](flecs::id _id) {
					if (!_id.is_pair())
					{
						std::string type = _id.entity().name().c_str();
						if (_display.find(type) != _display.end())
						{
							_display[type](current, type);
						}
					}
					});
			}

			if (ImGuiButtonWithAlign("Add Component", 0.5f))
			{
				ImGui::OpenPopup("Comp");
			}
			if (ImGui::BeginPopup("Comp"))
			{
				for (auto& i : _display)
				{
					if (ImGuiButtonWithAlign(i.first.c_str(), 0.5f))
					{
						_add[i.first](current);

					}
				}
				ImGui::EndPopup();
			}

		}
		ImGui::End();
	}
}
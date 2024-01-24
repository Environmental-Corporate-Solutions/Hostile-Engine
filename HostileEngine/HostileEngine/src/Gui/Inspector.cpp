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
			if (Input::IsTriggered(Key::Delete))
			{
				IEngine::Get().GetWorld().defer([&]() {current.destruct(); });
				IEngine::Get().GetGUI().SetSelectedObject(-1);
			}

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
			if (Input::IsPressed(Key::LeftControl) && Input::IsTriggered(Key::D))
			{
				flecs::entity copy = current.clone();
				std::string name = current.get_ref<ObjectName>()->name;
				name = GenerateDupName(name);
				copy.set<ObjectName>({ name });
				//REMOVE when sam fixes copy bug
				if (copy.has<Renderer>())
				{
					copy.get_mut<Renderer>()->m_id = copy.id();
				}
				world.entity(copy);
				IEngine::Get().GetGUI().SetSelectedObject(copy.id());
			}
			if (Input::IsPressed(Key::LeftControl) && Input::IsTriggered(Key::C))
			{
				m_clipboard = current.clone();
			}
			if (Input::IsPressed(Key::LeftControl) && Input::IsTriggered(Key::V))
			{
				flecs::entity copy = m_clipboard;
				std::string name = current.get_ref<ObjectName>()->name;
				name = GenerateDupName(name);
				copy.set<ObjectName>({ name });
				//REMOVE when sam fixes copy bug
				if (copy.has<Renderer>())
				{
					copy.get_mut<Renderer>()->m_id = copy.id();
				}
				world.entity(copy);
				IEngine::Get().GetGUI().SetSelectedObject(copy.id());
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
	std::string Inspector::GenerateDupName(std::string name)
	{
		int pos = name.find("(");
		int pos2 = name.find(")");
		if (pos == -1)
		{
			name += "(1)";
		}
		else
		{
			std::string value = name.substr(pos + 1, pos2 - 1);
			int num = std::stoi(value);
			++num;
			name = name.substr(0, pos);
			name += "(";
			name += std::to_string(num);
			name += ")";
		}
		return name;
	}
}
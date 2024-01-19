//------------------------------------------------------------------------------
//
// File Name:	Gui.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Gui.h"
#include "imgui.h"
#include "ImguiTheme.h"
#include "Engine.h"
#include "Script/ScriptEngine.h"
#include "misc/cpp/imgui_stdlib.h"

#include <commdlg.h>

#include "Graphics/IGraphics.h"
#include "Profiler/Profiler.h"

namespace Hostile
{
	void Gui::Init()
	{
		SetImGuiTheme();
		ImGuiIO io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.OversampleH = 5;
		icons_config.OversampleV = 5;

		ImFont* icons = io.Fonts->AddFontFromFileTTF("./Assets/Fonts/Font Awesome 6 Free-Solid-900.ttf", 10.5f, &icons_config, icons_ranges);

		m_explorer.Init();
	}
	void Gui::RenderGui()
	{

		ImGui::GetIO().FontGlobalScale = m_font_scale;
		MainMenuBar();


		Log::DrawConsole();
		Script::ScriptEngine::Draw();



		m_sceneVeiwer.Render(m_displayFuncs, m_addFuncs);
		m_explorer.Render();
		ImGui::Begin("Fps window");
		ImGui::Text("FPS: %.1f ", IEngine::Get().FrameRate());
		ImGui::End();

	}
	void Gui::RegisterComponent(const std::string& _name, DisplayFunc _display, AddFunc _add)
	{
		m_displayFuncs[_name] = _display;
		m_addFuncs[_name] = _add;
	}
	void Gui::SetSelectedObject(int _id)
	{
		m_sceneVeiwer.SetSelectedObject(_id);
	}
	int Gui::GetSelectedObject()
	{
		return m_sceneVeiwer.GetSelectedObject();
	}

	void Gui::MainMenuBar()
	{
		bool a = false;
		bool b = false;
		ImVec4 black = { 30 / 255.0f,30 / 255.0f,30 / 255.0f,1 };
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, black);

		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New"))
			{
				b = true;
			}
			if (ImGui::MenuItem("Save"))
			{
				ISceneManager::Get().GetCurrentScene()->Save();
			}
			if (ImGui::MenuItem("Save as"))
			{
				a = true;
			}
			ImGui::EndMenu();
			if (a)
			{
				ImGui::OpenPopup("###Save As");
			}
			if (b)
			{
				ImGui::OpenPopup("###New");
			}
		}
		if (ImGui::BeginPopup("###New"))
		{
			ImGui::InputText("Scene Name", &save_as_string);
			if (ImGui::Button("Create"))
			{
				ISceneManager::Get().AddScene(save_as_string, " ");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("###Save As"))
		{
			ImGui::InputText("File Name", &save_as_string);
			if (ImGuiButtonWithAlign("Save") && !save_as_string.empty())
			{
				IEngine& engine = IEngine::Get();
				ISceneManager& scene_manager = ISceneManager::Get();
				flecs::entity scene = engine.GetWorld().entity(scene_manager.GetCurrentScene()->Id());
				std::string temp = scene.get<ObjectName>()->name;
				scene.set<ObjectName>({ save_as_string });
				scene_manager.GetCurrentScene()->Save();
				scene.set<ObjectName>({ temp });
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}


		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::EndMenu();
		}
		//pos = ImGui::GetCursorPos();
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Graphics Settings", NULL, &m_graphics_settings);
			if (ImGui::MenuItem("Profiler"))
			{
				Profiler::OpenProfiler();
			}
			//ImGui::OpenPopup("###View");
			ImGui::EndMenu();
		}

        if (m_graphics_settings)
        {
            ImGui::Begin("Graphics Settings", &m_graphics_settings);
			ImGui::InputFloat("Font scale", &m_font_scale, 0.5f);
            if (ImGui::ColorEdit4("Ambient", &m_ambient_light.x))
            {
                IGraphics::Get().SetAmbientLight(m_ambient_light);
            }

			ImGui::End();
		}

		ImGui::EndMainMenuBar();
		ImGui::PopStyleColor();

	}
}

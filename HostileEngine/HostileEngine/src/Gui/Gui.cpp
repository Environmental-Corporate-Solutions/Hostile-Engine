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

namespace Hostile
{
	void Gui::Init()
	{
		m_explorer.Init();
	}
	void Gui::RenderGui()
	{
		ImGui::GetIO().FontGlobalScale = 1.75f;
		SetImGuiTheme();
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
		ImVec4 black = { 30 / 255.0f,30 / 255.0f,30 / 255.0f,1 };
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, black);

		ImGui::BeginMainMenuBar();

		ImGui::MenuItem("File");
		ImGui::MenuItem("Edit");
		ImGui::MenuItem("View");

		ImGui::EndMainMenuBar();
		ImGui::PopStyleColor();

	}
}

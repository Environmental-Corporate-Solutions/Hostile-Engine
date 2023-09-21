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
namespace Hostile
{
  void Gui::RenderGui()
  {
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGui::GetIO().FontGlobalScale = 1.75f;
    SetImGuiTheme();
    ImGui::Begin("Test");
    ImGui::Button("Hello");
    ImGui::SliderFloat("test slider", &gamer, 0, 2.5f);
    ImGui::InputFloat("Test input", &gamer);
    ImGui::Checkbox("bool", &thing1);
    ImGui::End();

    ImGui::Begin("hello world");
    ImGui::End();

    ImGui::Begin("Test2");
    ImGui::End();

    Log::DrawConsole();

    m_sceneVeiwer.Render();
  }
}
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

    Log::DrawConsole();
    Script::ScriptEngine::Draw();

    m_sceneVeiwer.Render();
    m_explorer.Render();
    ImGui::Begin("Fps window");
    ImGui::Text("FPS: %.1f ", IEngine::Get().FrameRate());
    ImGui::End();
    
  }
  void Gui::RegisterComponent(const std::string& _name, ISystemPtr _sys)
  {
    m_map[_name] = _sys;
  }
}

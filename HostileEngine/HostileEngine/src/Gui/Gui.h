//------------------------------------------------------------------------------
//
// File Name:	Gui.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "SceneViewer.h"
#include "FileExplorer.h"
#include "ISystemPtr.h"
namespace Hostile
{



  class Gui
  {
  public:
    void Init();
    void RenderGui();
    void RegisterComponent(const std::string& _name, DisplayFunc _display, AddFunc _add);

    void SetSelectedObject(int _id);
    int GetSelectedObject();
  private:
    bool thing1 = false;
    bool m_graphics_settings = false;
    bool m_profiler = false;
    DirectX::SimpleMath::Vector4 m_ambient_light = { 249 / 255.0f, 129 / 255.0f, 43 / 255.0f, 0.1f};
    SceneViewer m_sceneVeiwer;
    FileExplorer m_explorer;
    DisplayMap m_displayFuncs;
    AddMap m_addFuncs;
    float m_font_scale = 1.75f;
    std::string save_as_string;
    void MainMenuBar();

  };


}
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
    SceneViewer m_sceneVeiwer;
    FileExplorer m_explorer;
    DisplayMap m_displayFuncs;
    AddMap m_addFuncs;
  };


}
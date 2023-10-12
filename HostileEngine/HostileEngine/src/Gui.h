//------------------------------------------------------------------------------
//
// File Name:	Gui.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
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
    void RegisterComponent(const std::string& _name, ISystemPtr _sys);

  private:
    float gamer = 0;
    bool thing1 = false;
    SceneViewer m_sceneVeiwer;
    FileExplorer m_explorer;
    std::unordered_map<std::string, ISystemPtr> m_map;

  };


}
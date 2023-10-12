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
namespace Hostile
{
  class Gui
  {
  public:
    void Init();
    void RenderGui();

  private:
    float gamer = 0;
    bool thing1 = false;
    SceneViewer m_sceneVeiwer;
    FileExplorer m_explorer;

  };


}
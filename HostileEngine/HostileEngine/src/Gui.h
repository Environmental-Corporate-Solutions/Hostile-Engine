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
namespace Hostile
{
  class Gui
  {
  public:
    void RenderGui();

  private:
    float gamer = 0;
    bool thing1 = false;
    SceneViewer m_sceneVeiwer;
  };


}
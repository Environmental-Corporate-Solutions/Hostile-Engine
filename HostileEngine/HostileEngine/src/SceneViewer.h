//------------------------------------------------------------------------------
//
// File Name:	SceneViewer.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------


#include "flecs.h"
namespace Hostile
{
  class SceneViewer
  {
  public:
    void Render();
  private:
    int counter = 1;
    static void DisplayEntity(flecs::entity _entity,int* _id);
    static void DragAndDrop(flecs::entity _entity);
    static void DragAndDropRoot();
    int m_selected = -1;
    std::string m_name;
  };

}
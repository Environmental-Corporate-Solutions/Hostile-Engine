//------------------------------------------------------------------------------
//
// File Name:	SceneViewer.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "flecs.h"
#include "Inspector.h"
namespace Hostile
{
  class SceneViewer
  {
  public:
    void Render(DisplayMap& _display, AddMap& _add);
  private:
    Inspector m_inspector;
    int counter = 1;
    static void DisplayEntity(flecs::entity _entity,int* _id);
    static void DragAndDrop(flecs::entity _entity);
    static void DragAndDropRoot();
    int m_selected = -1;
    std::string m_name;
  };

}
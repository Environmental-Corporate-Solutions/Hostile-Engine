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
    void SetSelectedObject(int _id);
    inline int GetSelectedObject() { return m_selected; };
  private:
    Inspector m_inspector;
    int counter = 1;
    static void DisplayEntity(flecs::entity _entity,int* _id);
    static void DragAndDrop(flecs::entity _entity);
    static void DragAndDropRoot();
    void DisplayScene(flecs::entity _entity, int* _id);
    int m_selected = -1;
    std::string m_name;
    static flecs::entity* m_to_delete;
  };

}
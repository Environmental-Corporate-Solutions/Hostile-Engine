//------------------------------------------------------------------------------
//
// File Name:	SceneViewer.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include <vector>
#include <string>
#include "flecs.h"
namespace Hostile
{
  class SceneViewer
  {
  public:
    void Render();
  private:
    std::vector<std::string> m_entityList;
    std::vector<flecs::entity> m_entities;
    int counter = 1;
    int m_currentobj = -1;
    static void DisplayEntity(flecs::entity _entity);
  };

}
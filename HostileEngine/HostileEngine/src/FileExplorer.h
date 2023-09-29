//------------------------------------------------------------------------------
//
// File Name:	FileExplorer.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include <filesystem>
#include "imgui.h"
namespace Hostile
{
  class FileExplorer
  {
  public:
    void Init();
    void Render();
  private:
    void ShowFolder(std::filesystem::directory_entry _entry);


    std::filesystem::path m_current_path;
    std::filesystem::path m_root_path;
    ImFont* icons;
    std::filesystem::directory_entry m_entry;
    bool m_selected_this_frame = false;
  };
}
//------------------------------------------------------------------------------
//
// File Name:	FileExplorer.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
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
    void DisplayFile(std::filesystem::directory_entry _entry);
    void DisplayJSON(std::filesystem::directory_entry _entry);
    void DisplayFolder(std::filesystem::directory_entry _entry);
    void DisplayScene(std::filesystem::directory_entry _entry);
    void DoubleClickOpen(std::filesystem::directory_entry _entry);



    void MakeFile();
    void MakeScene(const std::string& _name);
    void MakeScript(const std::string& _name);
    

    std::string payload;
    std::filesystem::path m_current_path;
    std::filesystem::path m_root_path;
    std::string m_new_file_name;
    std::filesystem::directory_entry m_entry;
    bool m_selected_this_frame = false;
    bool m_scene_creation = false;
  };
}
//------------------------------------------------------------------------------
//
// File Name:	FileExplorer.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "FileExplorer.h"
#include <string>
#include"Input.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "font_awesome.h"
namespace fs = std::filesystem;
namespace Hostile
{
  void FileExplorer::Init()
  {
    fs::path path = fs::current_path();
    if (!fs::is_directory(path.string() + "/Content"))
    {
      fs::create_directories(path.string() + "/Content");
    }
    m_current_path = fs::path(path.string() + "/Content");

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.OversampleH = 3;
    icons_config.OversampleV = 3;

    icons = io.Fonts->AddFontFromFileTTF("./Assets/font.ttf", 10.0f, &icons_config, icons_ranges);



  }
  void FileExplorer::Render()
  {

    ImGui::Begin("File Explorer");
    ImGui::BeginTable("Files", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable);
    ImGui::TableSetupColumn("###files", ImGuiTableColumnFlags_WidthStretch,200.0f);
    ImGui::TableSetupColumn("###viewer", ImGuiTableColumnFlags_WidthStretch,200.0f);
    ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetWindowHeight());
    ImGui::TableSetColumnIndex(0);
    
    //std::string name = m_current_path.filename().string();
    std::string name = ICON_FA_FOLDER " ";
    name += m_current_path.filename().string();
    std::string id = "###";
    id += m_current_path.filename().string();
    if (ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str())))
    {
      name = ICON_FA_FOLDER_OPEN " ";
      name += m_current_path.filename().string();
    }
    if (ImGui::TreeNodeEx(id.c_str(),ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow,name.c_str()))
    {
      for (fs::directory_entry entry : fs::directory_iterator(m_current_path))
      {
        if (entry.is_directory())
        {
          ShowFolder(entry);
        }
      }
      ImGui::TreePop();
    }
    ImGui::EndTable();
    for (fs::directory_entry entry : fs::directory_iterator(m_current_path))
    {

    }
    ImGui::End();
  }
  void FileExplorer::ShowFolder(std::filesystem::directory_entry _entry)
  {

    std::string name = ICON_FA_FOLDER " ";
    name += _entry.path().filename().string();
    std::string id = "###";
    id += _entry.path().filename().string();
    if (ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str())))
    {
      name = ICON_FA_FOLDER_OPEN " ";
      name += _entry.path().filename().string();
    }
    
    if (ImGui::TreeNodeEx(id.c_str(),ImGuiTreeNodeFlags_OpenOnArrow,name.c_str()))
    {
      
      for (fs::directory_entry entry : fs::directory_iterator(_entry.path()))
      {
        if (entry.is_directory())
        {
          ShowFolder(entry);
        }
      }
      ImGui::TreePop();
    }
  }
}

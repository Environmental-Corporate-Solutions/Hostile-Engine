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
		m_root_path = fs::path(path.string() + "/Content");
		m_current_path = m_root_path;
		m_entry = fs::directory_entry(m_current_path);

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

		if (ImGui::Begin("File Explorer"))
		{
			ImGui::BeginTable("Files", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_Resizable);
			ImGui::TableSetupColumn("###files", ImGuiTableColumnFlags_WidthStretch, 200.0f);
			ImGui::TableSetupColumn("###viewer", ImGuiTableColumnFlags_WidthStretch, 200.0f);
			ImGui::TableNextRow(ImGuiTableRowFlags_None, ImGui::GetWindowHeight());
			ImGui::TableSetColumnIndex(0);

			//std::string name = m_current_path.filename().string();
			m_selected_this_frame = false;
			std::string name = ICON_FA_FOLDER " ";
			name += m_root_path.filename().string();
			std::string id = "###";
			id += m_root_path.filename().string();
			if (ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str())))
			{
				name = ICON_FA_FOLDER_OPEN " ";
				name += m_root_path.filename().string();
			}
			if (ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, name.c_str()))
			{
				for (fs::directory_entry entry : fs::directory_iterator(m_root_path))
				{
					if (entry.is_directory())
					{
						ShowFolder(entry);
					}
				}
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked() && !m_selected_this_frame)
			{
				m_current_path = m_root_path;
				m_selected_this_frame = true;
			}
			ImGui::TableSetColumnIndex(1);
			for (fs::directory_entry entry : fs::directory_iterator(m_current_path))
			{
				DisplayFile(entry);
				std::string compare = entry.path().filename().string();
				compare = compare.substr(entry.path().stem().string().size());
				if (compare == ".json")
				{
					DisplayJSON(entry);
				}
			}
			ImGui::EndTable();
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

		if (ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_None, name.c_str()))
		{
			for (fs::directory_entry entry : fs::directory_iterator(_entry.path()))
			{
				if (entry.is_directory())
				{
					ShowFolder(entry);
				}
			}
			if (ImGui::IsItemClicked() && !m_selected_this_frame)
			{
				m_current_path = _entry.path();
				m_selected_this_frame = true;
			}
			ImGui::TreePop();
		}
		if (ImGui::IsItemClicked() && !m_selected_this_frame)
		{
			m_current_path = _entry.path();
			m_selected_this_frame = true;
		}
	}
	void FileExplorer::DisplayFile(std::filesystem::directory_entry _entry)
	{
		ImGui::BeginGroup();
		

		ImGui::EndGroup();
		ImGui::Text(_entry.path().filename().string().c_str());
	}
	void FileExplorer::DisplayJSON(std::filesystem::directory_entry _entry)
	{
		ImGui::BeginGroup();
		std::string title = ICON_FA_BOOK;
		title += _entry.path().filename().string().c_str();
		ImGui::TreeNodeEx(title.c_str(),ImGuiTreeNodeFlags_Leaf);
		ImGui::TreePop();
		ImGui::EndGroup();

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			payload = _entry.path().string();
			ImGui::SetDragDropPayload("PREFAB", &payload, sizeof(payload));
			std::string name = _entry.path().filename().string();
			ImGui::Text(name.c_str());
			ImGui::EndDragDropSource();
		}
	}
}

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
#include "IconsFontAwesome6.h"
#include <shellapi.h>
#include "Engine.h"

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

				std::string compare = entry.path().filename().string();
				compare = compare.substr(entry.path().stem().string().size());
				if (compare == ".json")
				{
					DisplayJSON(entry);
				}
				else if (entry.is_directory())
				{
					DisplayFolder(entry);
				}
				else if (compare == ".scene")
				{
					DisplayScene(entry);
				}
				else
				{
					DisplayFile(entry);
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
		std::string title = ICON_FA_FILE_LINES" ";
		title += _entry.path().filename().string().c_str();
		ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_Leaf);
		ImGui::TreePop();
		ImGui::EndGroup();
		DoubleClickOpen(_entry);
	}
	void FileExplorer::DisplayJSON(std::filesystem::directory_entry _entry)
	{
		ImGui::BeginGroup();
		std::string title = ICON_FA_CUBE" ";
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
		DoubleClickOpen(_entry);
	}
	void FileExplorer::DisplayFolder(std::filesystem::directory_entry _entry)
	{
		ImGui::BeginGroup();
		
		std::string title = ICON_FA_FOLDER" ";
		title += _entry.path().filename().string().c_str();
		ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_Leaf);
		ImGui::TreePop();
		ImGui::EndGroup();
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			m_current_path = _entry.path();
			m_selected_this_frame = true;
		}
	}

	void FileExplorer::DisplayScene(std::filesystem::directory_entry _entry)
	{
		ImGui::BeginGroup();

		std::string title = ICON_FA_CUBES" ";
		title += _entry.path().filename().string().c_str();
		ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_Leaf);
		ImGui::TreePop();
		ImGui::EndGroup();
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (!IEngine::Get().IsSceneLoaded(_entry.path().string()))
			{
				IDeseralizer::Get().ReadFile(_entry.path().string().c_str());
			}
		}
	}

	void FileExplorer::DoubleClickOpen(std::filesystem::directory_entry _entry)
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			ShellExecute(0, 0, _entry.path().c_str(), NULL, NULL, SW_SHOW);
		}
	}
}

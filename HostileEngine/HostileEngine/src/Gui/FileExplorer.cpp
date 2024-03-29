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
#include <fstream>
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
			bool taken = false;
			bool scene = false;
			bool script = false;
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
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					taken = true;
					ImGui::OpenPopup(entry.path().string().c_str());
				}
				if (ImGui::BeginPopup(entry.path().string().c_str()))
				{
					if (ImGui::MenuItem("Delete"))
					{
						fs::remove(entry);
					}
					ImGui::EndPopup();
				}
			}
			ImGui::EndTable();
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !taken)
			{
				ImGui::OpenPopup("###new file");
			}
			if (ImGui::BeginPopup("###new file"))
			{
				if (ImGui::BeginMenu("New"))
				{
					if (ImGui::MenuItem("File"))
					{
						MakeFile();
					}
					if (ImGui::MenuItem("Scene"))
					{
						scene = true;
					}
					if (ImGui::MenuItem("Script"))
					{
						script = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
				if (scene)
				{
					ImGui::OpenPopup("###SceneCreation");
					m_new_file_name.clear();
				}
				if (script)
				{
					ImGui::OpenPopup("###ScriptPopup");
					m_new_file_name.clear();
				}
			}
			if (ImGui::BeginPopup("###ScriptPopup"))
			{
				ImGui::InputText("Script Name", &m_new_file_name);
				if (!m_new_file_name.empty() && (ImGui::Button("Create") || Input::IsTriggered(Key::Enter)))
				{
					MakeScript(m_new_file_name);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("###SceneCreation"))
			{
				ImGui::InputText("Scene Name", &m_new_file_name);
				if (!m_new_file_name.empty() && ImGui::Button("Create") || Input::IsTriggered(Key::Enter))
				{
					MakeScene(m_new_file_name);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
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
		ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_Leaf);
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
			bool val = !ISceneManager::Get().IsSceneLoaded(_entry.path().stem().string());
			if (val)
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
	void FileExplorer::MakeFile()
	{
		fs::path npath = m_current_path;
		npath /= "NewFile.txt";
		fs::path temp = npath;
		int count = 1;
		while (fs::exists(temp))
		{
			temp = m_current_path;
			temp /= npath.stem();
			temp += "(" + std::to_string(count) + ").txt";
			count++;
		}
		std::ofstream ofs(temp.string());
	}

	void FileExplorer::MakeScene(const std::string& _name)
	{
		fs::path npath = m_current_path;
		npath /= _name;
		npath += ".scene";
		fs::path temp = npath;
		int count = 1;
		while (fs::exists(temp))
		{
			temp = m_current_path;
			temp /= npath.stem();
			temp += "(" + std::to_string(count) + ").scene";

			count++;
		}
		std::ifstream in("./Assets/FileTemplates/SceneTemplate.txt");
		std::ofstream out(temp.string());
		std::string wordToReplace = "NAME";
		std::string wordToReplaceWith = _name;

		std::string line;
		std::size_t len = wordToReplace.length();
		while (getline(in, line))
		{
			while (true)
			{
				size_t pos = line.find(wordToReplace);
				if (pos != std::string::npos)
					line.replace(pos, len, wordToReplaceWith);
				else
					break;
			}

			out << line << '\n';
		}
	}
	void FileExplorer::MakeScript(const std::string& _name)
	{
		std::string name = _name;
		auto end_pos = std::remove(name.begin(), name.end(), ' ');
		name.erase(end_pos, name.end());
		//remove_if(_name.begin(), _name.end(), isspace);
		fs::path npath = m_current_path;
		npath /= name;
		npath += ".cs";
		if (fs::exists(npath))
		{
			Log::Error("File name Already exists");
		}

		std::ifstream in("./Assets/FileTemplates/ScriptTemplate.txt");
		std::ofstream out(npath.string());
		std::string wordToReplace = "NAME";
		std::string wordToReplaceWith = _name;
		
		std::string line;
		std::size_t len = wordToReplace.length();
		while (getline(in, line))
		{
			while (true)
			{
				size_t pos = line.find(wordToReplace);
				if (pos != std::string::npos)
					line.replace(pos, len, wordToReplaceWith);
				else
					break;
			}

			out << line << '\n';
		}
	}
}

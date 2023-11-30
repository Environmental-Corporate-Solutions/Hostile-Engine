//------------------------------------------------------------------------------
//
// File Name:	Stub.cpp
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "stdafx.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Engine.h"

namespace Hostile
{
	static 
	class SceneManager : public ISceneManager
	{
	public:
		Scene& AddScene(const std::string& _name, std::string _path)
		{
			std::string temp_path;
			if (_path == " ")
			{
				temp_path += "Content/Scenes/";
				temp_path += _name;
				temp_path += ".scene";
				_path = temp_path;
			}
			Scene temp(_name, _path);
			m_scenes[_name] = temp;
			return m_scenes[_name];
		}

		Scene& GetScene(const std::string& _name)
		{
			return m_scenes[_name];
		}

		Scene* GetCurrentScene()
		{
			return &m_scenes[m_current_scene];
		}

		bool IsSceneLoaded(const std::string& _name)
		{
			return m_scenes.find(_name) != m_scenes.end();
		}

		void SetCurrentScene(const std::string& _name)
		{
			m_current_scene = _name;
		}

		void UnloadScene(int _id)
		{
			auto iter = m_scenes.begin();
			while (iter != m_scenes.end())
			{
				if (iter->second.Id() == _id)
				{
					iter->second.Unload();
      				m_scenes.erase(iter->first);
					return;
				}
				iter++;
			}
		}

		void ReloadScenes()
		{
			IEngine::Get().GetGUI().SetSelectedObject(-1);
			std::vector<std::string> scenes;
			auto iter = m_scenes.begin();
			//save all file paths
			while (iter != m_scenes.end())
			{
				scenes.push_back(iter->second.Path());
				iter->second.Unload();
				iter++;
			}
			m_scenes.clear();
			IEngine::Get().GetWorld().progress();
			IEngine::Get().GetWorld().progress();
			//re-read scenes from files
			for (std::string current : scenes)
			{
				IDeseralizer::Get().ReadFile(current.c_str());
			}

		}

	private:

		//std::vector<std::string> m_loaded_scenes;
		std::unordered_map<std::string, Scene> m_scenes;
		std::string m_current_scene;
	};

	ISceneManager& ISceneManager::Get()
	{
		static SceneManager manager;
		return manager;
	}
}
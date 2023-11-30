//------------------------------------------------------------------------------
//
// File Name:	SceneManager.h
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "Scene.h"

namespace Hostile
{
	class ISceneManager
	{
	public:
		static ISceneManager& Get();
		virtual Scene& AddScene(const std::string& _name, std::string _path) = 0;
		virtual Scene& GetScene(const std::string& _name) = 0;
		virtual Scene* GetCurrentScene() = 0;
		virtual bool IsSceneLoaded(const std::string& _name) = 0;
		virtual void SetCurrentScene(const std::string& _name) = 0;
		virtual void UnloadScene(int _id) = 0;
		virtual void ReloadScenes() = 0;
		virtual void SaveAllScenes() = 0;
	};
}
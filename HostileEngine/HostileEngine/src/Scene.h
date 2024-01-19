//------------------------------------------------------------------------------
//
// File Name:	Stub.h
// Author(s):	
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#pragma once
#include "flecs.h"
#include <string>


namespace Hostile
{
	struct IsScene {};
	class Scene
	{
	public:
		Scene();
		Scene(std::string _name, std::string _path);
		void Save();
		void SetPause(bool _value);
		void Add(flecs::entity& _entity);
		void Unload();
		inline int Id() { return m_scene; };
		inline const std::string& Path() { return m_path; };
		inline std::string Name() { return name; };

	private:
		std::string name;
		std::string m_path;
		int m_scene;
	};
}
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
		Scene(std::string _name);
		void Save();
		void SetPause(bool _value);
		void Add(flecs::entity& _entity);

	private:
		std::string name;
		int m_scene;
	};
}
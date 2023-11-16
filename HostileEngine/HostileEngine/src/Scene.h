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


namespace Hostile
{
	class Scene
	{
	public :
		void Save();
	private:
		flecs::entity& scene;
	};
}
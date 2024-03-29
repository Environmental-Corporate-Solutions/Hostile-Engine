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
#include "Scene.h"
#include "Engine.h"
#include "TransformSys.h"
#include "PhysicsSys.h"
namespace Hostile
{
	Scene::Scene(std::string _name, std::string _path) :
		name(_name),
		m_scene(-1),
		m_path(_path)
	{
		flecs::entity& scene = IEngine::Get().GetWorld().entity();
		m_scene = scene.id();
		scene.add<IsScene>();
		scene.set<ObjectName>({ _name });
	}

	Scene::Scene() :
		name("NA"),
		m_scene(-1)
	{
	}

	void Hostile::Scene::Save()
	{
		ISerializer::Get().WriteSceneToFile(IEngine::Get().GetWorld().entity(m_scene));
	}



	void Scene::SetPause(bool _value)
	{

	}

	void Scene::Add(flecs::entity& _entity)
	{
		flecs::entity& scene = IEngine::Get().GetWorld().entity(m_scene);
		_entity.child_of(scene);

	}


	void Scene::Unload()
	{
		IEngine& engine = IEngine::Get();
		flecs::entity& scene = engine.GetWorld().entity(m_scene);
		flecs::world& world = engine.GetWorld();

		// clear collision data for all entities in the scene
		PhysicsSys::ClearCollisionEvents();

		world.defer([&]{scene.destruct(); });
	}

}
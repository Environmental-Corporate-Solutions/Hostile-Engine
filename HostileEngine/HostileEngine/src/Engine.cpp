//------------------------------------------------------------------------------
//
// File Name:	Engine.cpp
// Author(s):	Isaiah Dickison
//						
//
// Copyright ?2021 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Engine.h"
#include <vector>
#include "flecs.h"
#include "ISystem.h"
#include "Gui/Gui.h"
#include <unordered_map>


#include "TransformSys.h"

namespace Hostile
{
	class Engine :public IEngine
	{
	public:

		void Add(ISystemPtr _pSys) override
		{
			m_allSystems.push_back(_pSys);
		}

		void Init() override
		{
			/* (custom phases)
			 *  TransformSys -> PhysicsSys
			*/
			m_world = std::make_unique<flecs::world>();

			m_physicsPhase = m_world->entity()
				.add(flecs::Phase)
				.depends_on(flecs::OnUpdate);

			m_integratePhase = m_world->entity()
				.add(flecs::Phase)
				.depends_on(m_physicsPhase);

			for (ISystem* pSys : m_allSystems)
			{
				pSys->OnCreate(*m_world);
			}
			IDeseralizer::Get().ReadFile("Content/Scenes/Basic Scene.scene");
			ISceneManager::Get().SetCurrentScene("Basic Scene");

			m_game_pipeline = m_world->get_pipeline();
			m_editor_pipeline = m_world->pipeline().with(flecs::System).with<Editor>().build();
			m_gui.Init();
			SetGameRunning(m_is_game_running);
		}

		Engine()
		{

		}

		void Update()
		{
			m_world->progress();
			m_gui.RenderGui();
		}

		flecs::world& GetWorld() override
		{
			return *m_world;
		}

		flecs::entity& GetPhysicsPhase() override final {
			return m_physicsPhase;
		}

		float FrameRate()
		{
			++m_frames;
			m_frameTime += m_world->delta_time();
			if (m_frameTime > 1)
			{
				m_fps = m_frames / m_frameTime;
				m_frames = 0;
				m_frameTime = 0;
			}
			return m_fps;
		}

		Gui& GetGUI()
		{
			return m_gui;
		}

		const bool IsGameRunning()
		{
			return m_is_game_running;
		}

		void SetGameRunning(bool _state)
		{
			m_is_game_running = _state;
			if (_state)
			{
				m_world->set_pipeline(m_game_pipeline);
			}
			else
			{
				m_world->set_pipeline(m_editor_pipeline);

			}
		}

		flecs::entity CreateEntity(const std::string& _name = "New Actor")
		{
			int counter = 0;
			std::string name = _name;
			while (DoesNameExist(name))
			{
				name = _name;
				++counter;
				name += "(" + std::to_string(counter) + ")";
			}
			flecs::entity& new_entity = m_world->entity();
			new_entity.add<Transform>();
			ObjectName name_comp;
			name_comp.name = name;
			new_entity.set<ObjectName>({ name });
			return new_entity;
		}










	private:
		std::vector<ISystemPtr>m_allSystems;
		std::unique_ptr<flecs::world> m_world;
		Gui m_gui;

		float m_frames = 0;
		float m_frameTime = 0;
		float m_fps = 0;
		bool m_is_game_running = false;
		flecs::entity m_game_pipeline;
		flecs::entity m_editor_pipeline;

		


		flecs::entity m_physicsPhase;
		flecs::entity m_integratePhase;


		bool DoesNameExist(const std::string& _name)
		{
			bool val = m_world->lookup(_name.c_str()).is_valid();
			
			return val;
		}
	};




	IEngine& IEngine::Get()
	{
		static Engine engine;
		return engine;
	}

}
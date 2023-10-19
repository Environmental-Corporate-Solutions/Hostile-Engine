#pragma once
#include "stdafx.h"
#include "ISystem.h"

namespace Hostile
{
	//I want to make this script be set and added only using flecs set and add functions
	//need this restriction for speed
	struct ScriptComponent
	{
		std::string Name;
	};


	/**
	 * \brief Jun: This will call ScriptEngine functions
	 */
	class ScriptSys : public ISystem
	{
	public:
		~ScriptSys() override;
		
		void OnCreate(flecs::world& _world) override;

		void OnEvent(flecs::iter& _eventIter, size_t _entityID, ScriptComponent& _script);
		void OnUpdate(flecs::iter& _it, ScriptComponent* _script);
		void Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type);
		void Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type);
		void GuiDisplay(flecs::entity& _entity, const std::string& type);
	};
}
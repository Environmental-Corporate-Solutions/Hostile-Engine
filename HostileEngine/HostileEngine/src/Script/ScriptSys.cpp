#include "stdafx.h"
#include "ScriptSys.h"
#include "Engine.h"
#include "Graphics/GraphicsSystem.h"
#include "Rigidbody.h"
#include "ScriptEngine.h"

namespace Hostile
{
	ADD_SYSTEM(ScriptSys);
	ScriptSys::~ScriptSys()
	{
	}

	void ScriptSys::OnCreate(flecs::world& _world)
	{
		IEngine::Get().GetGUI().RegisterComponent(
			"ScriptComponent",
			std::bind(&ScriptSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[](flecs::entity& _entity) {_entity.add<ScriptComponent>(); });

		_world.observer<ScriptComponent>("OnSetScript").event(flecs::OnSet)
		.each([&](flecs::iter& _eventIter, size_t _entityID, ScriptComponent& _script)
		{
			OnEvent(_eventIter, _entityID, _script);
		});

		_world.system<ScriptComponent>("ScriptUpdate").kind(flecs::OnUpdate).iter([&](flecs::iter& _it, ScriptComponent* _script)
			{ OnUpdate(_it, _script); });
		//testing
		auto player = _world.entity("player");
		player.set_name("player").set<Transform>({
				{0.f, 0.f, 0.f},
				{Quaternion::CreateFromAxisAngle(Vector3::UnitY, 0.f) },
				{10.f, 10.f, 10.f} })
				.set<ScriptComponent>({ "Test" });
        _world.entity("Light").set<ScriptComponent>({ "Light" });

	}

	void ScriptSys::OnEvent(flecs::iter& _eventIter, size_t _entityID, ScriptComponent& _script)
	{
		auto entity = _eventIter.entity(_entityID);
		//Log::Critical("entity {} : {}", entity.name(), _script.Name);

		Script::ScriptEngine::OnCreateEntity(entity);
	}

	void ScriptSys::OnUpdate(flecs::iter& _it, ScriptComponent* _script)
	{
		for (const int i:_it)
		{
			Script::ScriptEngine::OnUpdateEntity(_it.entity(i));
		}
	}

	void ScriptSys::Write(const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
	{

	}
	void ScriptSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
	}
	void ScriptSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		
		static std::function<void(const char*)> HelpMarker = [](const char* str)
			{
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(str);
			};

		if (ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ScriptComponent* scriptComp = _entity.get_mut<ScriptComponent>();
			static char scriptName[100] = { 0, };
			std::strcpy(scriptName, scriptComp->Name.c_str());
			bool scriptClassExists = Script::ScriptEngine::EntityClassExists(scriptComp->Name);
			if (!scriptClassExists)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.9f, 0.2f, 0.3f,1.f });

			if(ImGui::InputText("Class Name", scriptName, 100))
			{
				//must use set since I am assigning the class handle with flecs observer
				_entity.set<ScriptComponent>({ scriptName });
			}

			if (!scriptClassExists)
				ImGui::PopStyleColor();
			ImGui::SameLine();
			HelpMarker("Should include if there is namespace ex) namespace.classname \nIf there is no namespace then just classname.");
			ImGui::TreePop();
		}
		
	}
}

#include "stdafx.h"
#include "ScriptSys.h"
#include "Engine.h"
#include "Graphics/GraphicsSystem.h"
#include "PhysicsProperties.h"
#include "ScriptClass.h"
#include "ScriptEngine.h"

namespace Hostile
{
	ADD_SYSTEM(ScriptSys);
	ScriptSys::~ScriptSys()
	{
	}

	void ScriptSys::OnCreate(flecs::world& _world)
	{
		REGISTER_TO_SERIALIZER(ScriptComponent, this);
		REGISTER_TO_DESERIALIZER(ScriptComponent, this);

		IEngine::Get().GetGUI().RegisterComponent(
			"ScriptComponent",
			std::bind(&ScriptSys::GuiDisplay, this, std::placeholders::_1, std::placeholders::_2),
			[](flecs::entity& _entity) {_entity.add<ScriptComponent>(); });

		/*_world.observer<ScriptComponent>("OnSetScript").event(flecs::OnSet)
		.each([&](flecs::iter& _eventIter, size_t _entityID, ScriptComponent& _script)
		{
			OnEvent(_eventIter, _entityID, _script);
		});*/

		_world.system<ScriptComponent>("ScriptUpdate").kind(flecs::OnUpdate).iter([&](flecs::iter& _it, ScriptComponent* _script)
			{ OnUpdate(_it, _script); });
	}

	void ScriptSys::OnEvent(flecs::iter& _eventIter, size_t _entityID, ScriptComponent& _script)
	{
		auto entity = _eventIter.entity(_entityID);
		//Log::Critical("entity {} : {}", entity.name(), _script.Name);

		//Script::ScriptEngine::OnCreateEntity(_script.Name,entity);
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
		const ScriptComponent& temp = *_entity.get<ScriptComponent>();
		auto obj = nlohmann::json::object();
		obj["Type"] = "ScriptComponent";
		//obj["ClassName"] = temp.Name;
		_components.push_back(obj);
	}
	void ScriptSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
	{
		ScriptComponent scriptComponent;
		//scriptComponent.Name = _data["ClassName"];
		_object.set<ScriptComponent>(scriptComponent);
	}
	void ScriptSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
	{
		
		static std::function<void(const char*)> HelpMarker = [](const char* str)
			{
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(str);
			};
		bool is_open = ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("Script Popup");
		}
		if (ImGui::BeginPopup("Script Popup"))
		{
			if (ImGui::Button("Remove Component"))
			{
				_entity.remove<ScriptComponent>();
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			ImGui::EndPopup();
		}
		if (is_open)
		{
			ScriptComponent* scriptComp = _entity.get_mut<ScriptComponent>();
			static char scriptName[100] = { 0, };
			//std::strcpy(scriptName, scriptComp->Name.c_str());
			/*bool scriptClassExists = Script::ScriptEngine::EntityClassExists(scriptName);
			if (!scriptClassExists)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.9f, 0.2f, 0.3f,1.f });

			ImGui::InputText("Class Name", scriptName, 100);*/

			if(ImGui::Button("Add Script"))
			{
				ImGui::OpenPopup("Script Add PopUP");
				/*if(scriptClassExists)
				{
					Script::ScriptEngine::OnCreateEntity(scriptName, _entity);
				}*/
			}

			if (ImGui::BeginPopup("Script Add PopUP"))
			{
				auto& entityClasses = Script::ScriptEngine::GetEntityClasses();
				for (auto& [key, val] : entityClasses)
				{
					if (ImGui::Button(key.c_str()))
					{
						Script::ScriptEngine::OnCreateEntity(key, _entity);
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndPopup();
			}

			/*if (!scriptClassExists)
				ImGui::PopStyleColor();*/
			ImGui::SameLine();
			HelpMarker("Should include if there is namespace ex) namespace.classname \nIf there is no namespace then just classname.");

			{
				//auto& actualScript=Script::ScriptEngine::GetEntityClasses()[scriptComp->Name];
				for (auto uuid : scriptComp->UUIDs)
				{
					ImGui::Text("%s", Script::ScriptEngine::GetEntityScriptInstanceName(uuid).c_str());
					auto& actualScript = Script::ScriptEngine::GetEntityScriptInstance(uuid);
					auto& fields = actualScript->GetScriptClass()->GetFields();

					for (auto& [fieldName, field] : fields)
					{
						auto imguiName = fieldName + "##" + std::to_string(uuid);
						if (field == Script::ScriptFieldType::Int)
						{
							int data = actualScript->GetFieldValue<int>(fieldName);
							if (ImGui::DragInt(imguiName.c_str(), &data))
							{
								actualScript->SetFieldValue(fieldName, data);
							}
						}
						else if (field == Script::ScriptFieldType::Float)
						{
							float data = actualScript->GetFieldValue<float>(fieldName);
							if (ImGui::DragFloat(imguiName.c_str(), &data))
							{
								actualScript->SetFieldValue(fieldName, data);
							}
						}
						else if (field == Script::ScriptFieldType::Bool)
						{
							bool data = actualScript->GetFieldValue<bool>(fieldName);
							if (ImGui::Checkbox(imguiName.c_str(), &data))
							{
								actualScript->SetFieldValue(fieldName, data);
							}
						}
					}
				}
			}
			ImGui::Spacing();
		}
		
	}
}

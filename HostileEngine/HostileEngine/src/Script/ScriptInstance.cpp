#include "stdafx.h"
#include "ScriptInstance.h"
#include "ScriptClass.h"
#include <flecs.h>

#include "ScriptEngine.h"

namespace Script
{
	ScriptInstance::ScriptInstance(std::shared_ptr<ScriptClass> scriptClass, flecs::entity _entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		auto& entityClass = ScriptEngine::GetEntityClass();

		m_Constructor = entityClass.GetMethod(".ctor", 1);

		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		if (!m_OnCreateMethod)
			m_OnCreateMethod = entityClass.GetMethod("OnCreate", 0);

		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 0);
		if (!m_OnUpdateMethod)
			m_OnUpdateMethod = entityClass.GetMethod("OnUpdate", 0);
		
		{//call entity ctor
			uint64_t entityID = _entity.raw_id();
			void* params[1] =
			{
				(void*)&entityID,
			};
			m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, params);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod, nullptr);
	}

	void ScriptInstance::InvokeOnUpdate()
	{
		m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, nullptr);
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto found = fields.find(name);
		if (found == fields.end())
			return false;
		mono_field_get_value(m_Instance, found->second.ClassField, buffer);
		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto found = fields.find(name);
		if (found == fields.end())
			return false;
		mono_field_set_value(m_Instance, found->second.ClassField, (void*)value);
		return true;
	}
}

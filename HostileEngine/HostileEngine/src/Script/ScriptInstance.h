#pragma once
namespace flecs
{
	struct entity;
}

namespace Script
{
	class ScriptEngine;
	class ScriptClass;

	class ScriptInstance
	{
		friend class ScriptEngine;
	public:
		ScriptInstance(std::shared_ptr<ScriptClass> scriptClass, flecs::entity _entity);

		void InvokeOnCreate();
		void InvokeOnUpdate();

		std::shared_ptr<ScriptClass> GetScriptClass() { return m_ScriptClass; }

		template<typename T>
		T GetFieldValue(const std::string& name)
		{
			char fieldValueBuffer[8] = { 0, };
			bool success = GetFieldValueInternal(name, fieldValueBuffer);
			if (!success)
				return T();
			return *(T*)fieldValueBuffer;
		}

		template<typename T>
		void SetFieldValue(const std::string& name, T value)
		{
			SetFieldValueInternal(name, &value);
		}
	private:
		bool GetFieldValueInternal(const std::string& name, void* buffer);
		bool SetFieldValueInternal(const std::string& name, const void* value);
	private:

		std::shared_ptr<ScriptClass> m_ScriptClass;

		MonoObject* m_Instance = nullptr;
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
	};
}

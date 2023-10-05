#pragma once
#include "stdafx.h"
#include "ScriptField.h"

namespace Script
{
	class ScriptEngine;

	class ScriptClass
	{
		friend ScriptEngine;
	public:
		ScriptClass() = default;
		ScriptClass(MonoImage* assemblyImage, const std::string& nameSpace, const std::string& className);
		MonoObject* Instantiate();

		MonoMethod* GetMethod(const std::string& methodName, int paramCount);
		MonoObject* InvokeMethod(MonoObject* classInstance, MonoMethod* method, void** params);

		const std::map<std::string, ScriptField>& GetFields() const { return m_Fields; }
	private:
		std::string m_NameSpace;
		std::string m_ClassName;
		MonoClass* m_MonoClass;

		std::map<std::string, ScriptField> m_Fields;
	};
}
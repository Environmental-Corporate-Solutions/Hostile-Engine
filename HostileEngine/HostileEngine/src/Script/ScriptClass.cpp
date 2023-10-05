#include "stdafx.h"
#include "ScriptClass.h"
#include "ScriptEngine.h"
namespace Script
{
	ScriptClass::ScriptClass(MonoImage* assemblyImage, const std::string& nameSpace, const std::string& className)
		:m_NameSpace(nameSpace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(assemblyImage, nameSpace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& methodName, int paramCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, methodName.c_str(), paramCount);
	}

	MonoObject* Script::ScriptClass::InvokeMethod(MonoObject* classInstance, MonoMethod* method, void** params)
	{
		return mono_runtime_invoke(method, classInstance, params, nullptr);
	}
}
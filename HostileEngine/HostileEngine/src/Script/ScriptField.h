#pragma once
#include "ScriptEngine.h"
#include "ScriptInstance.h"

namespace Script
{
	enum class ScriptFieldType
	{
		None = 0,
		Float, Double,
		Bool,
		Char,
		Byte, Short, Int, Long,
		UShort, UInt, ULong,
		Vector2, Vector3, Vector4,
		Entity,
	};

	class ScriptField
	{
		friend ScriptEngine;
		friend ScriptInstance;
	public:
		ScriptField() = default;
		ScriptField(MonoType* monoType, const char* fieldName, MonoClassField* field);
		bool operator==(const ScriptFieldType& type) const;
		operator ScriptFieldType() const;
	private:
		ScriptFieldType Type;
		std::string Name;
		MonoClassField* ClassField;
	};
}

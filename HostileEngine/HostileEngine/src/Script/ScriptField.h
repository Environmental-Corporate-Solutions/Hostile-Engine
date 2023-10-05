#pragma once
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

	struct ScriptField
	{
		ScriptFieldType Type;
		std::string Name;
		MonoClassField* ClassField;
	};
}
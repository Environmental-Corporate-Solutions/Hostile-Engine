#include "stdafx.h"
#include "ScriptField.h"

namespace Script
{
	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap
	{
		{ "System.Single", ScriptFieldType::Float },
		{ "System.Double", ScriptFieldType::Double },

		{ "System.Boolean", ScriptFieldType::Bool },

		{ "System.Char", ScriptFieldType::Char },

		{ "System.Byte", ScriptFieldType::Byte },
		{ "System.Int16", ScriptFieldType::Short },
		{ "System.Int32", ScriptFieldType::Int },
		{ "System.Int64", ScriptFieldType::Long },

		{ "System.UInt16", ScriptFieldType::UShort },
		{ "System.UInt32", ScriptFieldType::UInt },
		{ "System.UInt64", ScriptFieldType::ULong },


		{ "HostileEngine.Vector2", ScriptFieldType::Vector2 },
		{ "HostileEngine.Vector3", ScriptFieldType::Vector3 },

	};

	ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
	{
		std::string typeName = mono_type_get_name(monoType);

		auto found = s_ScriptFieldTypeMap.find(typeName);
		if (found == s_ScriptFieldTypeMap.end())
		{
			Log::Error("Undefined type : {}", typeName);
			return ScriptFieldType::None;
		}
		return found->second;
	}

	const char* ScriptFieldTypeToString(ScriptFieldType type)
	{
		switch (type)
		{
		case ScriptFieldType::Float: return "Float";
		case ScriptFieldType::Double: return "Double";

		case ScriptFieldType::Bool: return "Bool";
		case ScriptFieldType::Char: return "Char";

		case ScriptFieldType::Byte: return "Byte";
		case ScriptFieldType::Short: return "Short";
		case ScriptFieldType::Int: return "Int";
		case ScriptFieldType::Long: return "Long";

		case ScriptFieldType::UShort: return "UShort";
		case ScriptFieldType::UInt: return "UInt";
		case ScriptFieldType::ULong: return "ULong";

		case ScriptFieldType::Vector2: return "Vector2";
		case ScriptFieldType::Vector3: return "Vector3";
		case ScriptFieldType::Vector4: return "Vector4";

		case ScriptFieldType::Entity: return "Entity";
		}
		return "Undefined Type Name";
	}

	ScriptField::ScriptField(MonoType* monoType, const char* fieldName, MonoClassField* field)
		:Name(fieldName), ClassField(field)
	{
		Type = MonoTypeToScriptFieldType(monoType);
		
		const char* typeName = ScriptFieldTypeToString(Type);
		Log::Info("- Public ({}) {}", typeName, fieldName);
	}

	bool ScriptField::operator==(const ScriptFieldType& type) const
	{
		return Type == type;
	}

	ScriptField::operator ScriptFieldType() const
	{
		return Type;
	}
}

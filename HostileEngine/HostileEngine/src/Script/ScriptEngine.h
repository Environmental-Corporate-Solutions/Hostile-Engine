#pragma once
#include "stdafx.h"

extern "C" {
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoString MonoString;
	typedef struct _MonoType MonoType;
}

namespace Script
{
	class ScriptEngine
	{
	public:
		static void Init(char* programArg);
		static void Shutdown();
	private:
		static void SetMonoAssembliesPath(const std::filesystem::path& programArg);
		static void InitMono();
		static void ShutdownMono();
	};
}
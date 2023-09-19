#pragma once

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
		static void Init(char* _programArg);
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& _relFilepath);
		//static void LoadAppAssembly(const std::filesystem::path& filepath);

	private:
		static void SetMonoAssembliesPath(const std::filesystem::path& _programArg);
		static void InitMono();
		static void ShutdownMono();

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& _assemblyPath);
		static void PrintAssemblyTypes(MonoAssembly* _assembly);
	};
}
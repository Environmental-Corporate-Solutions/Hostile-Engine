#include "stdafx.h"
#include "ScriptEngine.h"

#include <iostream>

namespace __ScriptEngineInner
{
	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		
	};

	static ScriptEngineData s_Data;

	static std::string AppDomainName = "HostileEngine_AppDomain";
}

namespace Script
{
	using namespace __ScriptEngineInner;
	
	void ScriptEngine::Init(char* programArg)
	{
		SetMonoAssembliesPath(programArg);
		InitMono();
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
	}

	void ScriptEngine::SetMonoAssembliesPath(const std::filesystem::path& programArg)
	{
		auto finalPath = programArg.parent_path() / "mono"/"lib";
		mono_set_assemblies_path(finalPath.string().c_str());
	}

	void ScriptEngine::InitMono()
	{
		s_Data.RootDomain = mono_jit_init("HostileEngine_JITRuntime");
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), true);

		if (s_Data.AppDomain)
		{
			mono_domain_unload(s_Data.AppDomain);
			s_Data.AppDomain = nullptr;
		}

		if (s_Data.RootDomain)
		{
			mono_jit_cleanup(s_Data.RootDomain);
			s_Data.RootDomain = nullptr;
		}
	}
}

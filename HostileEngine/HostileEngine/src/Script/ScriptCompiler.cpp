#include "stdafx.h"
#include "ScriptCompiler.h"
namespace __ScriptCompilerInner
{
	static MonoAssembly* s_CompilerAssembly = nullptr;
	static MonoImage* s_CompilerImage = nullptr;
	static MonoMethod* s_CompileFunc = nullptr;
	static std::filesystem::path s_ProgramPath;
}
namespace Script
{
	using namespace __ScriptCompilerInner;
	void ScriptCompiler::CompileAllCSFiles()
	{
		//Log::Debug("CompileAllCSFiles : {}", s_ProgramPath.string());
		Compile(s_ProgramPath.string());
	}

	void ScriptCompiler::Init(MonoAssembly* _compilerAssembly, std::filesystem::path _programPath)
	{
		s_CompilerAssembly = _compilerAssembly;
		s_CompilerImage = mono_assembly_get_image(s_CompilerAssembly);
		s_ProgramPath = _programPath;
		Log::Debug("ScriptCompiler Init : {}", s_ProgramPath.string());
		MonoClass* compilerClass = mono_class_from_name(s_CompilerImage, "HostileEngine", "Compiler");
		s_CompileFunc = mono_class_get_method_from_name(compilerClass, "Compile", 2);
	}

	void ScriptCompiler::Compile(const std::string& basePath)
	{
		mono_thread_attach(mono_domain_get());
		if (s_CompileFunc != nullptr)
		{
			void* args[] = {
				mono_string_new(mono_domain_get(), basePath.c_str()),
				mono_string_new(mono_domain_get(), (std::filesystem::path(basePath)/ "mono" / "lib" / "mono" / "4.5").string().c_str())
			};
			mono_runtime_invoke(s_CompileFunc, nullptr, args, nullptr);
		}
	}
}

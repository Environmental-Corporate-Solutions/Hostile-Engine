#pragma once
namespace Script
{
	class ScriptEngine;
	class ScriptCompiler
	{
		friend ScriptEngine;
	public:
		static void CompileAllCSFiles();
	private:
		static void Init(MonoAssembly* _compilerAssembly, std::filesystem::path _programPath);
		static void Compile(const std::string& basePath);
	};
}
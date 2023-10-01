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
		static bool Compile(const std::string& basePath);
		static void DrawConsole();
	};
}
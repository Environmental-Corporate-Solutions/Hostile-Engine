#include "stdafx.h"
#include "ScriptCompiler.h"
#include "imgui.h"
#include "ScriptEngine.h"
#include <vector>
#include <string>
namespace Script
{
	class ScriptCompilerConsole
	{
		friend ScriptEngine;
		friend ScriptCompiler;
	public:
		static void Add(MonoString* monoString)
		{
			char* to_print = mono_string_to_utf8(monoString);
			std::string str_to_print = to_print;
			mono_free(to_print);
			AddString(str_to_print);
		}

		static void AddString(const std::string& str)
		{
			s_Logs.push_back(str);
		}

		static void Clear()
		{
			s_Logs.clear();
		}
		static void Draw(bool result)
		{
			ImGui::Separator();

			if(ImGui::BeginChild("compiler_scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

				constexpr ImVec4 red(255, 0, 0, 255);
				constexpr ImVec4 green(0, 255, 0, 255);

				const ImVec4 colorToUse = result ? green : red;
				ImGui::PushStyleColor(ImGuiCol_Text, colorToUse);

				for (auto& s:s_Logs)
				{
					ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetWindowContentRegionMax().x);
					ImGui::Text(s.c_str());
					ImGui::PopTextWrapPos();
				}
					

				ImGui::PopStyleColor();
				ImGui::PopStyleVar();

				// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
				// Using a scrollbar or mouse-wheel will take away from the bottom edge.
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}
	private:
		static std::vector<std::string> s_Logs;
	};

	std::vector<std::string> ScriptCompilerConsole::s_Logs;

	static MonoAssembly* s_CompilerAssembly = nullptr;
	static MonoImage* s_CompilerImage = nullptr;
	static MonoMethod* s_CompileFunc = nullptr;
	static std::filesystem::path s_ProgramPath;

	static bool s_LastCompilationResult = false;

	bool ScriptCompiler::CompileAllCSFiles()
	{
		Log::Debug("Compile Start!");

		s_LastCompilationResult = Compile(s_ProgramPath.string());

		if (s_LastCompilationResult)
		{
			Log::Debug("Compile Success!");
			return true;
		}
		
		Log::Warn("Compile Failed");
		return false;
	}

	void ScriptCompiler::Init(MonoAssembly* _compilerAssembly, std::filesystem::path _programPath)
	{
		s_CompilerAssembly = _compilerAssembly;
		s_CompilerImage = mono_assembly_get_image(s_CompilerAssembly);
		s_ProgramPath = _programPath;

		Log::Debug("ScriptCompiler Init : {}", s_ProgramPath.string());

		{//ScriptCompilerConsole Stuff
			mono_add_internal_call("HostileEngine.CompilerConsoleInternalCalls::WriteLine", ScriptCompilerConsole::Add);
		}

		MonoClass* compilerClass = mono_class_from_name(s_CompilerImage, "HostileEngine", "Compiler");
		s_CompileFunc = mono_class_get_method_from_name(compilerClass, "Compile", 2);
	}

	bool ScriptCompiler::Compile(const std::string& basePath)
	{
		//mono_thread_attach(mono_domain_get());
		if (s_CompileFunc != nullptr)
		{
			void* args[] = {
				mono_string_new(mono_domain_get(), basePath.c_str()),
				mono_string_new(mono_domain_get(), (std::filesystem::path(basePath)/ "mono" / "lib" / "mono" / "4.5").string().c_str())
			};
			MonoObject* result = mono_runtime_invoke(s_CompileFunc, nullptr, args, nullptr);

			const int int_result = *(int*)mono_object_unbox(result);

			return int_result == 0;
		}
		return false;
	}

	void ScriptCompiler::DrawConsole()
	{
		if (ImGui::Button("Compile & Reload"))
		{
			ScriptCompilerConsole::Clear();
			if(CompileAllCSFiles()) //reload only when we get compile success 
				Script::ScriptEngine::Reload();
		}
		ScriptCompilerConsole::Draw(s_LastCompilationResult);
	}
}

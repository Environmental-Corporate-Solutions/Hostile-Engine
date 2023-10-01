#include "stdafx.h"
#include "ScriptCompiler.h"
#include "imgui.h"

namespace Script
{
	class ScriptCompilerConsole
	{
		friend ScriptEngine;
		friend ScriptCompiler;
	public:
		static void Add(MonoString* monoString)
		{
			int old_size = Buf.size();
			{
				std::string str_to_print;
				{
					char* to_print = mono_string_to_utf8(monoString);
					str_to_print = to_print;
					mono_free(to_print);
				}
				str_to_print += '\n';

				Buf.append(str_to_print.c_str());
			}
			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);
		}
		static void Clear()
		{
			Buf.clear();
			LineOffsets.clear();
			LineOffsets.push_back(0);
		}
		static void Draw(bool result)
		{
			ImGui::Separator();

			if (ImGui::BeginChild("compiler_scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

				constexpr ImVec4 red(255, 0, 0, 255);
				constexpr ImVec4 green(0, 255, 0, 255);

				const ImVec4 colorToUse = result ? green : red;
				ImGui::PushStyleColor(ImGuiCol_Text, colorToUse);

				const char* buf = Buf.begin();
				const char* buf_end = Buf.end();

				ImGuiListClipper clipper;
				clipper.Begin(LineOffsets.Size);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + LineOffsets[line_no];
						const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetWindowContentRegionMax().x);
						
						ImGui::TextUnformatted(line_start, line_end);
						
						ImGui::PopTextWrapPos();
					}
				}
				clipper.End();

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
		static ImGuiTextBuffer     Buf;
		static ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		static bool                AutoScroll;  // Keep scrolling if already at the bottom.
	};

	ImGuiTextBuffer ScriptCompilerConsole::Buf;
	ImVector<int> ScriptCompilerConsole::LineOffsets;
	bool ScriptCompilerConsole::AutoScroll = true;

	static MonoAssembly* s_CompilerAssembly = nullptr;
	static MonoImage* s_CompilerImage = nullptr;
	static MonoMethod* s_CompileFunc = nullptr;
	static std::filesystem::path s_ProgramPath;

	static bool s_LastCompilationResult = false;

	void ScriptCompiler::CompileAllCSFiles()
	{
		s_LastCompilationResult = Compile(s_ProgramPath.string());

		if(s_LastCompilationResult)
			Log::Debug("Compile Success!");
		else
			Log::Warn("Compile Failed");
	}

	void ScriptCompiler::Init(MonoAssembly* _compilerAssembly, std::filesystem::path _programPath)
	{
		s_CompilerAssembly = _compilerAssembly;
		s_CompilerImage = mono_assembly_get_image(s_CompilerAssembly);
		s_ProgramPath = _programPath;

		Log::Debug("ScriptCompiler Init : {}", s_ProgramPath.string());

		{//ScriptCompilerConsole Stuff
			ScriptCompilerConsole::Clear();
			mono_add_internal_call("HostileEngine.CompilerConsole::WriteLine", ScriptCompilerConsole::Add);
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
		if (ImGui::Button("Compile"))
		{
			ScriptCompilerConsole::Clear();
			CompileAllCSFiles();
		}
		ScriptCompilerConsole::Draw(s_LastCompilationResult);
	}
}

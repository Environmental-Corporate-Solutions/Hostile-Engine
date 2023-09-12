#include "stdafx.h"
#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <imgui.h>


namespace _LogInternal
{
	class LoggerBuffer
	{
	public:
		void Add(const std::string& str)
		{
			int old_size = Buf.size();
			Buf.append(str.c_str());

			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);
		}

		void Clear()
		{
			Buf.clear();
			LineOffsets.clear();
			LineOffsets.push_back(0);
		}

		void Draw()
		{
			ImGui::Begin("Console");
			// Options menu
			if (ImGui::BeginPopup("Options"))
			{
				ImGui::Checkbox("Auto-scroll", &AutoScroll);
				ImGui::EndPopup();
			}

			// Main window
			if (ImGui::Button("Options"))
				ImGui::OpenPopup("Options");

			ImGui::SameLine();
			bool clear = ImGui::Button("Clear");
			ImGui::Separator();

			if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
			{
				if (clear)
					Clear();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
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

				ImGui::PopStyleVar();

				// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
				// Using a scrollbar or mouse-wheel will take away from the bottom edge.
				if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
			ImGui::End();
		}

	private:
		ImGuiTextBuffer     Buf;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		bool                AutoScroll = true;  // Keep scrolling if already at the bottom.
	};
	static LoggerBuffer s_buffer;

	class ImGuiSink : public spdlog::sinks::base_sink<std::mutex>
	{
	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			// log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
			// msg.raw contains pre formatted log

			// If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
			s_buffer.Add(fmt::to_string(formatted));
		}

		void flush_() override {}
	};
}

static std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name);
std::shared_ptr<spdlog::logger> Log::s_EngineLogger = CreateLogger("Hostile Engine");

static std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name)
{
	std::vector<spdlog::sink_ptr> logSinks;

	/*
	//if we want to use dup filter
	auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_st>(std::chrono::seconds(5));
	dup_filter->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(dup_filter);
	*/

	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks[0]->set_pattern("%^[%T] %n: %v%$");
	//for later use
	logSinks.emplace_back(std::make_shared<_LogInternal::ImGuiSink>());
	logSinks[1]->set_pattern("%^[%T] %n: [%l] %v%$");

	//for later use
	//logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.txt", true));
	//logSinks[2]->set_pattern("[%T] [%l] %n: %v");

	std::shared_ptr<spdlog::logger> Logger = std::make_shared<spdlog::logger>(name, begin(logSinks), end(logSinks));
	spdlog::register_logger(Logger);
	Logger->set_level(spdlog::level::trace);
	Logger->flush_on(spdlog::level::trace);
	return Logger;
}

void Log::DrawConsole()
{
	_LogInternal::s_buffer.Draw();
}


#include "stdafx.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

static std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name)
{
	std::vector<spdlog::sink_ptr> logSinks;

	/*
	//if we want to use dup filter
	auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_st>(std::chrono::seconds(5));
	dup_filter->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(dup_filter);
	*/

	//for later use
	//logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.txt", true));
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks[0]->set_pattern("%^[%T] %n: %v%$");
	//for later use
	//logSinks[1]->set_pattern("[%T] [%l] %n: %v");

	std::shared_ptr<spdlog::logger> Logger = std::make_shared<spdlog::logger>(name, begin(logSinks), end(logSinks));
	spdlog::register_logger(Logger);
	Logger->set_level(spdlog::level::trace);
	Logger->flush_on(spdlog::level::trace);
	return Logger;
}

std::shared_ptr<spdlog::logger> Log::s_EngineLogger = CreateLogger("Hostile Engine");

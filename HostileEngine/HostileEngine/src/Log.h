#pragma once
#include "stdafx.h"

class Log
{
public:
	template <typename... Args>
	static void Trace(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->trace(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Trace(const T& msg)
	{
		s_EngineLogger->trace(msg);
	}

	template <typename... Args>
	static void Info(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->info(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Info(const T& msg)
	{
		s_EngineLogger->info(msg);
	}

	template <typename... Args>
	static void Warn(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->warn(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Warn(const T& msg)
	{
		s_EngineLogger->warn(msg);
	}

	template <typename... Args>
	static void Error(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->error(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Error(const T& msg)
	{
		s_EngineLogger->error(msg);
	}

	template <typename... Args>
	static void Critical(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->critical(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Critical(const T& msg)
	{
		s_EngineLogger->critical(msg);
	}

	template <typename... Args>
	static void Debug(spdlog::format_string_t<Args...> fmt, Args &&... args)
	{
		s_EngineLogger->debug(fmt, std::forward<Args>(args)...);
	}
	template<typename T>
	static void Debug(const T& msg)
	{
		s_EngineLogger->debug(msg);
	}

	static void DrawConsole();
private:
	static std::shared_ptr<spdlog::logger> s_EngineLogger;
};

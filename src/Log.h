#pragma once

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <memory>
#pragma warning(pop)

namespace OpenGL {
	class Log
	{
	public:
		static void Init();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
	};
};

// Core log macros
#ifdef GL_DEBUG
	#define GL_TRACE(...)    ::OpenGL::Log::GetCoreLogger()->trace(__VA_ARGS__)
	#define GL_INFO(...)     ::OpenGL::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define GL_WARN(...)     ::OpenGL::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define GL_ERROR(...)    ::OpenGL::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define GL_CRITICAL(...) ::OpenGL::Log::GetCoreLogger()->critical(__VA_ARGS__)
#endif

#ifndef GL_DEBUG
	#define GL_TRACE
	#define GL_INFO
	#define GL_WARN
	#define GL_ERROR
	#define GL_CRITICAL
#endif



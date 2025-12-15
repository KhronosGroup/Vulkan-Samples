/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#	include <dbghelp.h>
#	include <windows.h>
#	pragma comment(lib, "dbghelp.lib")
#elif defined(__APPLE__) || defined(__linux__)
#	include <execinfo.h>
#	include <signal.h>
#	include <unistd.h>
#endif

#include "debug_system.h"

/**
 * @brief Class for crash reporting and minidump generation.
 *
 * This class implements the crash reporting system as described in the Tooling chapter:
 * @see en/Building_a_Simple_Engine/Tooling/04_crash_minidump.adoc
 */
class CrashReporter
{
  public:
	/**
	 * @brief Get the singleton instance of the crash reporter.
	 * @return Reference to the crash reporter instance.
	 */
	static CrashReporter &GetInstance()
	{
		static CrashReporter instance;
		return instance;
	}

	/**
	 * @brief Initialize the crash reporter.
	 * @param minidumpDir The directory to store minidumps.
	 * @param appName The name of the application.
	 * @param appVersion The version of the application.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &minidumpDir = "crashes",
	                const std::string &appName     = "SimpleEngine",
	                const std::string &appVersion  = "1.0.0")
	{
		std::lock_guard<std::mutex> lock(mutex);

		this->minidumpDir = minidumpDir;
		this->appName     = appName;
		this->appVersion  = appVersion;

// Create minidump directory if it doesn't exist
#ifdef _WIN32
		CreateDirectoryA(minidumpDir.c_str(), NULL);
#else
		std::string command = "mkdir -p " + minidumpDir;
		system(command.c_str());
#endif

		// Install crash handlers
		InstallCrashHandlers();

		// Register with debug system
		DebugSystem::GetInstance().SetCrashHandler([this](const std::string &message) {
			this->HandleCrash(message);
		});

		LOG_INFO("CrashReporter", "Crash reporter initialized");
		initialized = true;
		return true;
	}

	/**
	 * @brief Clean up crash reporter resources.
	 */
	void Cleanup()
	{
		std::lock_guard<std::mutex> lock(mutex);

		if (initialized)
		{
			// Uninstall crash handlers
			UninstallCrashHandlers();

			LOG_INFO("CrashReporter", "Crash reporter shutting down");
			initialized = false;
		}
	}

	/**
	 * @brief Handle a crash.
	 * @param message The crash message.
	 */
	void HandleCrash(const std::string &message)
	{
		std::lock_guard<std::mutex> lock(mutex);

		LOG_FATAL("CrashReporter", "Crash detected: " + message);

		// Generate minidump
		GenerateMinidump(message);

		// Call registered callbacks
		for (const auto &callback : crashCallbacks)
		{
			callback(message);
		}
	}

	/**
	 * @brief Register a crash callback.
	 * @param callback The callback function to be called when a crash occurs.
	 * @return An ID that can be used to unregister the callback.
	 */
	int RegisterCrashCallback(std::function<void(const std::string &)> callback)
	{
		std::lock_guard<std::mutex> lock(mutex);

		int id             = nextCallbackId++;
		crashCallbacks[id] = callback;
		return id;
	}

	/**
	 * @brief Unregister a crash callback.
	 * @param id The ID of the callback to unregister.
	 */
	void UnregisterCrashCallback(int id)
	{
		std::lock_guard<std::mutex> lock(mutex);

		crashCallbacks.erase(id);
	}

	/**
	 * @brief Generate a minidump.
	 * @param message The crash message.
	 */
	void GenerateMinidump(const std::string &message)
	{
		// Get current time for filename
		auto now  = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		char timeStr[20];
		std::strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", std::localtime(&time));

		// Create minidump filename
		std::string filename = minidumpDir + "/" + appName + "_" + timeStr + ".dmp";

		LOG_INFO("CrashReporter", "Generating minidump: " + filename);

// Generate minidump based on platform
#ifdef _WIN32
		// Windows implementation
		HANDLE hFile = CreateFileA(
		    filename.c_str(),
		    GENERIC_WRITE,
		    0,
		    NULL,
		    CREATE_ALWAYS,
		    FILE_ATTRIBUTE_NORMAL,
		    NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION exInfo;
			exInfo.ThreadId          = GetCurrentThreadId();
			exInfo.ExceptionPointers = NULL;        // Would be set in a real exception handler
			exInfo.ClientPointers    = FALSE;

			MiniDumpWriteDump(
			    GetCurrentProcess(),
			    GetCurrentProcessId(),
			    hFile,
			    MiniDumpNormal,
			    &exInfo,
			    NULL,
			    NULL);

			CloseHandle(hFile);
		}
#else
		// Unix implementation
		std::ofstream file(filename, std::ios::out | std::ios::binary);
		if (file.is_open())
		{
			// Get backtrace
			void  *callstack[128];
			int    frames  = backtrace(callstack, 128);
			char **symbols = backtrace_symbols(callstack, frames);

			// Write header
			file << "Crash Report for " << appName << " " << appVersion << std::endl;
			file << "Timestamp: " << timeStr << std::endl;
			file << "Message: " << message << std::endl;
			file << std::endl;

			// Write backtrace
			file << "Backtrace:" << std::endl;
			for (int i = 0; i < frames; i++)
			{
				file << symbols[i] << std::endl;
			}

			free(symbols);
			file.close();
		}
#endif

		LOG_INFO("CrashReporter", "Minidump generated: " + filename);
	}

  private:
	// Private constructor for singleton
	CrashReporter() = default;

	// Delete copy constructor and assignment operator
	CrashReporter(const CrashReporter &)            = delete;
	CrashReporter &operator=(const CrashReporter &) = delete;

	// Mutex for thread safety
	std::mutex mutex;

	// Initialization flag
	bool initialized = false;

	// Minidump directory
	std::string minidumpDir = "crashes";

	// Application info
	std::string appName    = "SimpleEngine";
	std::string appVersion = "1.0.0";

	// Crash callbacks
	std::unordered_map<int, std::function<void(const std::string &)>> crashCallbacks;
	int                                                               nextCallbackId = 0;

	/**
	 * @brief Install platform-specific crash handlers.
	 */
	void InstallCrashHandlers()
	{
#ifdef _WIN32
		// Windows implementation
		SetUnhandledExceptionFilter([](EXCEPTION_POINTERS *exInfo) -> LONG {
			CrashReporter::GetInstance().HandleCrash("Unhandled exception");
			return EXCEPTION_EXECUTE_HANDLER;
		});
#else
		// Unix implementation
		signal(SIGSEGV, [](int sig) {
			CrashReporter::GetInstance().HandleCrash("Segmentation fault");
			exit(1);
		});

		signal(SIGABRT, [](int sig) {
			CrashReporter::GetInstance().HandleCrash("Abort");
			exit(1);
		});

		signal(SIGFPE, [](int sig) {
			CrashReporter::GetInstance().HandleCrash("Floating point exception");
			exit(1);
		});

		signal(SIGILL, [](int sig) {
			CrashReporter::GetInstance().HandleCrash("Illegal instruction");
			exit(1);
		});
#endif
	}

	/**
	 * @brief Uninstall platform-specific crash handlers.
	 */
	void UninstallCrashHandlers()
	{
#ifdef _WIN32
		// Windows implementation
		SetUnhandledExceptionFilter(NULL);
#else
		// Unix implementation
		signal(SIGSEGV, SIG_DFL);
		signal(SIGABRT, SIG_DFL);
		signal(SIGFPE, SIG_DFL);
		signal(SIGILL, SIG_DFL);
#endif
	}
};

// Convenience macro for simulating a crash (for testing)
#define SIMULATE_CRASH(message) CrashReporter::GetInstance().HandleCrash(message)

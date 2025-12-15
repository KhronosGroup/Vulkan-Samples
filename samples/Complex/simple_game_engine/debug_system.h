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
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @brief Enum for different log levels.
 */
enum class LogLevel
{
	Debug,
	Info,
	Warning,
	Error,
	Fatal
};

/**
 * @brief Class for managing debugging and logging.
 *
 * This class implements the debugging system as described in the Tooling chapter:
 * @see en/Building_a_Simple_Engine/Tooling/03_debugging_and_renderdoc.adoc
 */
class DebugSystem
{
  public:
	/**
	 * @brief Get the singleton instance of the debug system.
	 * @return Reference to the debug system instance.
	 */
	static DebugSystem &GetInstance()
	{
		static DebugSystem instance;
		return instance;
	}

	/**
	 * @brief Initialize the debug system.
	 * @param logFilePath The path to the log file.
	 * @return True if initialization was successful, false otherwise.
	 */
	bool Initialize(const std::string &logFilePath = "engine.log")
	{
		std::lock_guard<std::mutex> lock(mutex);

		// Open log file
		logFile.open(logFilePath, std::ios::out | std::ios::trunc);
		if (!logFile.is_open())
		{
			std::cerr << "Failed to open log file: " << logFilePath << std::endl;
			return false;
		}

		// Log initialization
		Log(LogLevel::Info, "DebugSystem", "Debug system initialized");

		initialized = true;
		return true;
	}

	/**
	 * @brief Clean up debug system resources.
	 */
	void Cleanup()
	{
		std::lock_guard<std::mutex> lock(mutex);

		if (initialized)
		{
			// Log cleanup
			Log(LogLevel::Info, "DebugSystem", "Debug system shutting down");

			// Close log file
			if (logFile.is_open())
			{
				logFile.close();
			}

			initialized = false;
		}
	}

	/**
	 * @brief Log a message.
	 * @param level The log level.
	 * @param tag The tag for the log message.
	 * @param message The log message.
	 */
	void Log(LogLevel level, const std::string &tag, const std::string &message)
	{
		std::lock_guard<std::mutex> lock(mutex);

		// Get current time
		auto now  = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		char timeStr[20];
		std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&time));

		// Format log message
		std::string levelStr;
		switch (level)
		{
			case LogLevel::Debug:
				levelStr = "DEBUG";
				break;
			case LogLevel::Info:
				levelStr = "INFO";
				break;
			case LogLevel::Warning:
				levelStr = "WARNING";
				break;
			case LogLevel::Error:
				levelStr = "ERROR";
				break;
			case LogLevel::Fatal:
				levelStr = "FATAL";
				break;
		}

		std::string formattedMessage =
		    std::string(timeStr) + "." + std::to_string(ms.count()) +
		    " [" + levelStr + "] " +
		    "[" + tag + "] " +
		    message;

		// Write to console
		if (level >= LogLevel::Warning)
		{
			std::cerr << formattedMessage << std::endl;
		}
		else
		{
			std::cout << formattedMessage << std::endl;
		}

		// Write to log file
		if (logFile.is_open())
		{
			logFile << formattedMessage << std::endl;
			logFile.flush();
		}

		// Call registered callbacks
		for (const auto &kv : logCallbacks)
		{
			kv.second(level, tag, message);
		}

		// If fatal, trigger crash handler
		if (level == LogLevel::Fatal && crashHandler)
		{
			crashHandler(formattedMessage);
		}
	}

	/**
	 * @brief Register a log callback.
	 * @param callback The callback function to be called when a log message is generated.
	 * @return An ID that can be used to unregister the callback.
	 */
	int RegisterLogCallback(std::function<void(LogLevel, const std::string &, const std::string &)> callback)
	{
		std::lock_guard<std::mutex> lock(mutex);

		int id           = nextCallbackId++;
		logCallbacks[id] = callback;
		return id;
	}

	/**
	 * @brief Unregister a log callback.
	 * @param id The ID of the callback to unregister.
	 */
	void UnregisterLogCallback(int id)
	{
		std::lock_guard<std::mutex> lock(mutex);

		logCallbacks.erase(id);
	}

	/**
	 * @brief Set the crash handler.
	 * @param handler The crash handler function.
	 */
	void SetCrashHandler(std::function<void(const std::string &)> handler)
	{
		std::lock_guard<std::mutex> lock(mutex);

		crashHandler = handler;
	}

	/**
	 * @brief Start a performance measurement.
	 * @param name The name of the measurement.
	 */
	void StartMeasurement(const std::string &name)
	{
		std::lock_guard<std::mutex> lock(mutex);

		auto now           = std::chrono::high_resolution_clock::now();
		measurements[name] = now;
	}

	/**
	 * @brief End a performance measurement and log the result.
	 * @param name The name of the measurement.
	 */
	void StopMeasurement(const std::string &name)
	{
		auto                        now = std::chrono::high_resolution_clock::now();
		std::lock_guard<std::mutex> lock(mutex);

		auto it = measurements.find(name);

		if (it != measurements.end())
		{
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - it->second).count();
			Log(LogLevel::Debug, "Performance", name + ": " + std::to_string(duration) + " us");
			measurements.erase(it);
		}
		else
		{
			Log(LogLevel::Error, "Performance", "No measurement started with name: " + name);
		}
	}

  protected:
	// Protected constructor for inheritance
	DebugSystem()          = default;
	virtual ~DebugSystem() = default;

	// Delete copy constructor and assignment operator
	DebugSystem(const DebugSystem &)            = delete;
	DebugSystem &operator=(const DebugSystem &) = delete;

	// Mutex for thread safety
	std::mutex mutex;

	// Log file
	std::ofstream logFile;

	// Initialization flag
	bool initialized = false;

	// Log callbacks
	std::unordered_map<int, std::function<void(LogLevel, const std::string &, const std::string &)>> logCallbacks;
	int                                                                                              nextCallbackId = 0;

	// Crash handler
	std::function<void(const std::string &)> crashHandler;

	// Performance measurements
	std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> measurements;
};

// Convenience macros for logging
#define LOG_DEBUG(tag, message) DebugSystem::GetInstance().Log(LogLevel::Debug, tag, message)
#define LOG_INFO(tag, message) DebugSystem::GetInstance().Log(LogLevel::Info, tag, message)
#define LOG_WARNING(tag, message) DebugSystem::GetInstance().Log(LogLevel::Warning, tag, message)
#define LOG_ERROR(tag, message) DebugSystem::GetInstance().Log(LogLevel::Error, tag, message)
#define LOG_FATAL(tag, message) DebugSystem::GetInstance().Log(LogLevel::Fatal, tag, message)

// Convenience macros for performance measurement
#define MEASURE_START(name) DebugSystem::GetInstance().StartMeasurement(name)
#define MEASURE_END(name) DebugSystem::GetInstance().StopMeasurement(name)

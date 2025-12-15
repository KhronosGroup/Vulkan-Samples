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
#include "renderdoc_debug_system.h"

#include <cstring>
#include <iostream>

#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#	include <dlfcn.h>
#endif

// Value for eRENDERDOC_API_Version_1_4_1 from RenderDoc's header to avoid including it
#ifndef RENDERDOC_API_VERSION_1_4_1
#	define RENDERDOC_API_VERSION_1_4_1 10401
#endif

// Minimal local typedefs and struct to receive function pointers without including renderdoc_app.h
using pTriggerCaptureLocal    = void (*)();
using pStartFrameCaptureLocal = void (*)(void *, void *);
using pEndFrameCaptureLocal   = unsigned int (*)(void *, void *);

struct RENDERDOC_API_1_4_1_MIN
{
	pTriggerCaptureLocal    TriggerCapture;
	void                   *_pad0;        // We don't rely on layout beyond the subset we read via memcpy
	pStartFrameCaptureLocal StartFrameCapture;
	pEndFrameCaptureLocal   EndFrameCapture;
};

bool RenderDocDebugSystem::LoadRenderDocAPI()
{
	if (renderdocAvailable)
		return true;

	// Try to fetch RENDERDOC_GetAPI from a loaded module without forcing a dependency
	pRENDERDOC_GetAPI getAPI = nullptr;

#if defined(_WIN32)
	HMODULE mod = GetModuleHandleA("renderdoc.dll");
	if (!mod)
	{
		// If not already injected/loaded, do not force-load. We can attempt LoadLibraryA as a fallback
		mod = LoadLibraryA("renderdoc.dll");
		if (!mod)
		{
			LOG_INFO("RenderDoc", "RenderDoc not loaded into process");
			return false;
		}
	}
	getAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
#elif defined(__APPLE__) || defined(__linux__)
	void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
	if (!mod)
	{
		// Try to load if not already loaded; if unavailable, just no-op
		mod = dlopen("librenderdoc.so", RTLD_NOW);
		if (!mod)
		{
			LOG_INFO("RenderDoc", "RenderDoc not loaded into process");
			return false;
		}
	}
	getAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
#endif

	if (!getAPI)
	{
		LOG_WARNING("RenderDoc", "RENDERDOC_GetAPI symbol not found");
		return false;
	}

	// Request API 1.4.1 into a temporary buffer and then extract needed functions
	RENDERDOC_API_1_4_1_MIN apiMin{};
	void                   *apiPtr = nullptr;
	int                     result = getAPI(RENDERDOC_API_VERSION_1_4_1, &apiPtr);
	if (result == 0 || apiPtr == nullptr)
	{
		LOG_WARNING("RenderDoc", "Failed to acquire RenderDoc API 1.4.1");
		return false;
	}

	// Copy only the subset we care about; layout is stable for these early members
	std::memcpy(&apiMin, apiPtr, sizeof(apiMin));

	fnTriggerCapture    = apiMin.TriggerCapture;
	fnStartFrameCapture = apiMin.StartFrameCapture;
	fnEndFrameCapture   = apiMin.EndFrameCapture;

	renderdocAvailable = (fnTriggerCapture || fnStartFrameCapture || fnEndFrameCapture);

	if (renderdocAvailable)
	{
		LOG_INFO("RenderDoc", "RenderDoc API loaded");
	}
	else
	{
		LOG_WARNING("RenderDoc", "RenderDoc API did not provide expected functions");
	}

	return renderdocAvailable;
}

void RenderDocDebugSystem::TriggerCapture()
{
	if (!renderdocAvailable && !LoadRenderDocAPI())
		return;
	if (fnTriggerCapture)
	{
		fnTriggerCapture();
		LOG_INFO("RenderDoc", "Triggered capture");
	}
	else
	{
		LOG_WARNING("RenderDoc", "TriggerCapture not available");
	}
}

void RenderDocDebugSystem::StartFrameCapture(void *device, void *window)
{
	if (!renderdocAvailable && !LoadRenderDocAPI())
		return;
	if (fnStartFrameCapture)
	{
		fnStartFrameCapture(device, window);
		LOG_DEBUG("RenderDoc", "StartFrameCapture called");
	}
	else
	{
		LOG_WARNING("RenderDoc", "StartFrameCapture not available");
	}
}

bool RenderDocDebugSystem::EndFrameCapture(void *device, void *window)
{
	if (!renderdocAvailable && !LoadRenderDocAPI())
		return false;
	if (fnEndFrameCapture)
	{
		unsigned int ok = fnEndFrameCapture(device, window);
		LOG_DEBUG("RenderDoc", ok ? "EndFrameCapture succeeded" : "EndFrameCapture failed");
		return ok != 0;
	}
	LOG_WARNING("RenderDoc", "EndFrameCapture not available");
	return false;
}

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
#include "camera_component.h"
#include "engine.h"
#include "scene_loading.h"
#include "transform_component.h"

#include <iostream>
#include <stdexcept>
#include <thread>

// Constants
constexpr int WINDOW_WIDTH  = 800;
constexpr int WINDOW_HEIGHT = 600;
#if defined(NDEBUG)
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

/**
 * @brief Set up a simple scene with a camera and some objects.
 * @param engine The engine to set up the scene in.
 */
void SetupScene(Engine *engine)
{
	// Create a camera entity
	Entity *cameraEntity = engine->CreateEntity("Camera");
	if (!cameraEntity)
	{
		throw std::runtime_error("Failed to create camera entity");
	}

	// Add a transform component to the camera
	auto *cameraTransform = cameraEntity->AddComponent<TransformComponent>();
	cameraTransform->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));

	// Add a camera component to the camera entity
	auto *camera = cameraEntity->AddComponent<CameraComponent>();
	camera->SetAspectRatio(static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT));

	// Set the camera as the active camera
	engine->SetActiveCamera(camera);

	// Kick off GLTF model loading on a background thread so the main loop
	// can start and render the UI/progress bar while the scene is being
	// constructed. Engine::Update will avoid updating entities while
	// loading is in progress to prevent data races.
	if (auto *renderer = engine->GetRenderer())
	{
		renderer->SetLoading(true);
	}
	std::thread([engine] {
		// Use the animated Bistro variant with ceiling fan animations (merged from bistrox.gltf and bistro_fans.gltf)
		LoadGLTFModel(engine, "../Assets/bistro/bistrox_with_fans.gltf");
	}).detach();
}

#if defined(PLATFORM_ANDROID)
/**
 * @brief Android entry point.
 * @param app The Android app.
 */
void android_main(android_app *app)
{
	try
	{
		// Create the engine
		Engine engine;

		// Initialize the engine
		if (!engine.InitializeAndroid(app, "Simple Engine", ENABLE_VALIDATION_LAYERS))
		{
			throw std::runtime_error("Failed to initialize engine");
		}

		// Set up the scene
		SetupScene(&engine);

		// Run the engine
		engine.RunAndroid();
	}
	catch (const std::exception &e)
	{
		LOGE("Exception: %s", e.what());
	}
}
#else
/**
 * @brief Desktop entry point.
 * @return The exit code.
 */
int main(int, char *[])
{
	try
	{
		// Create the engine
		Engine engine;

		// Initialize the engine
		if (!engine.Initialize("Simple Engine", WINDOW_WIDTH, WINDOW_HEIGHT, ENABLE_VALIDATION_LAYERS))
		{
			throw std::runtime_error("Failed to initialize engine");
		}

		// Set up the scene
		SetupScene(&engine);

		// Run the engine
		engine.Run();

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}
#endif

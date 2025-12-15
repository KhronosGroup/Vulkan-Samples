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

#include "entity.h"
#include <iostream>

// Most of the CameraComponent class implementation is in the header file
// This file is mainly for any methods that might need additional implementation
//
// This implementation corresponds to the Camera_Transformations chapter in the tutorial:
// @see en/Building_a_Simple_Engine/Camera_Transformations/03_camera_implementation.adoc

// Initializes the camera by updating the view and projection matrices
// @see en/Building_a_Simple_Engine/Camera_Transformations/03_camera_implementation.adoc#camera-initialization
void CameraComponent::Initialize()
{
	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

// Returns the view matrix, updating it if necessary
// @see en/Building_a_Simple_Engine/Camera_Transformations/03_camera_implementation.adoc#accessing-camera-matrices
const glm::mat4 &CameraComponent::GetViewMatrix()
{
	if (viewMatrixDirty)
	{
		UpdateViewMatrix();
	}
	return viewMatrix;
}

// Returns the projection matrix, updating it if necessary
// @see en/Building_a_Simple_Engine/Camera_Transformations/03_camera_implementation.adoc#accessing-camera-matrices
const glm::mat4 &CameraComponent::GetProjectionMatrix()
{
	if (projectionMatrixDirty)
	{
		UpdateProjectionMatrix();
	}
	return projectionMatrix;
}

// Updates the view matrix based on the camera's position and orientation
// @see en/Building_a_Simple_Engine/Camera_Transformations/04_transformation_matrices.adoc#view-matrix
void CameraComponent::UpdateViewMatrix()
{
	auto transformComponent = owner->GetComponent<TransformComponent>();
	if (transformComponent)
	{
		// Build camera world transform (T * R) from the camera entity's transform
		// and compute the view matrix as its inverse. This ensures consistency
		// with rasterization and avoids relying on an external target vector.
		const glm::vec3 position = transformComponent->GetPosition();
		const glm::vec3 euler    = transformComponent->GetRotation();        // radians

		const glm::quat qx = glm::angleAxis(euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::quat qy = glm::angleAxis(euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::quat qz = glm::angleAxis(euler.z, glm::vec3(0.0f, 0.0f, 1.0f));
		const glm::quat q  = qz * qy * qx;        // match TransformComponent's ZYX composition

		const glm::mat4 T            = glm::translate(glm::mat4(1.0f), position);
		const glm::mat4 R            = glm::mat4_cast(q);
		const glm::mat4 worldNoScale = T * R;

		viewMatrix = glm::inverse(worldNoScale);
	}
	else
	{
		// Fallback: default camera at origin looking towards +Z with Y up
		// Note: keep consistent with right-handed convention used elsewhere
		const glm::vec3 position(0.0f);
		const glm::vec3 forward(0.0f, 0.0f, 1.0f);
		const glm::vec3 upVec(0.0f, 1.0f, 0.0f);
		viewMatrix = glm::lookAt(position, position + forward, upVec);
	}
	viewMatrixDirty = false;
}

// Updates the projection matrix based on the camera's projection type and parameters
// @see en/Building_a_Simple_Engine/Camera_Transformations/04_transformation_matrices.adoc#projection-matrix
void CameraComponent::UpdateProjectionMatrix()
{
	if (projectionType == ProjectionType::Perspective)
	{
		projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
	}
	else
	{
		float halfWidth  = orthoWidth * 0.5f;
		float halfHeight = orthoHeight * 0.5f;
		projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
	}
	projectionMatrixDirty = false;
}

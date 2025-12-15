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
#include "animation_component.h"
#include "entity.h"
#include "transform_component.h"

#include <algorithm>
#include <iostream>

void AnimationComponent::Update(std::chrono::milliseconds deltaTime)
{
	if (!playing || currentAnimationIndex < 0 ||
	    currentAnimationIndex >= static_cast<int>(animations.size()))
	{
		return;
	}

	const Animation &anim     = animations[currentAnimationIndex];
	float            duration = anim.GetDuration();

	if (duration <= 0.0f)
	{
		return;
	}

	// Advance time
	float dt = static_cast<float>(deltaTime.count()) * 0.001f * playbackSpeed;
	currentTime += dt;

	// Handle looping or stopping at the end
	if (currentTime >= duration)
	{
		if (looping)
		{
			currentTime = std::fmod(currentTime, duration);
		}
		else
		{
			currentTime = duration;
			playing     = false;
		}
	}

	// Capture base transforms on first update if not already captured
	if (basePositions.empty())
	{
		for (const auto &[nodeIndex, entity] : nodeToEntity)
		{
			if (entity)
			{
				auto *transform = entity->GetComponent<TransformComponent>();
				if (transform)
				{
					basePositions[nodeIndex] = transform->GetPosition();
					// Convert Euler angles to quaternion for proper rotation composition
					glm::vec3 eulerAngles    = transform->GetRotation();
					baseRotations[nodeIndex] = glm::quat(eulerAngles);
					baseScales[nodeIndex]    = transform->GetScale();
				}
			}
		}
	}

	// Apply animation to all channels
	for (const auto &channel : anim.channels)
	{
		if (channel.samplerIndex < 0 ||
		    channel.samplerIndex >= static_cast<int>(anim.samplers.size()))
		{
			continue;
		}

		const AnimationSampler &sampler = anim.samplers[channel.samplerIndex];

		// Find the target entity for this node
		auto entityIt = nodeToEntity.find(channel.targetNode);
		if (entityIt == nodeToEntity.end() || !entityIt->second)
		{
			continue;
		}

		Entity *targetEntity = entityIt->second;
		auto   *transform    = targetEntity->GetComponent<TransformComponent>();
		if (!transform)
		{
			continue;
		}

		// Get base transform for this node (defaults to identity if not found)
		glm::vec3 basePos   = basePositions.count(channel.targetNode) ?
		                          basePositions[channel.targetNode] :
		                          glm::vec3(0.0f);
		glm::quat baseRot   = baseRotations.count(channel.targetNode) ?
		                          baseRotations[channel.targetNode] :
		                          glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 baseScale = baseScales.count(channel.targetNode) ?
		                          baseScales[channel.targetNode] :
		                          glm::vec3(1.0f);

		// Sample and apply the appropriate transform property
		// Animation transforms are applied RELATIVE to the base transform
		switch (channel.path)
		{
			case AnimationPath::Translation:
			{
				glm::vec3 animTranslation = SampleVec3(sampler, currentTime);
				// Apply animation translation relative to base position
				transform->SetPosition(basePos + animTranslation);
				break;
			}
			case AnimationPath::Rotation:
			{
				glm::quat animRotation = SampleQuat(sampler, currentTime);
				// Compose rotations using quaternion multiplication (order matters!)
				// Final rotation = base rotation * animation delta rotation
				glm::quat finalRotation = baseRot * animRotation;
				// Convert to Euler angles for the transform component
				transform->SetRotation(glm::eulerAngles(finalRotation));
				break;
			}
			case AnimationPath::Scale:
			{
				glm::vec3 animScale = SampleVec3(sampler, currentTime);
				// Multiply scales (animation scale is a factor, not an offset)
				transform->SetScale(baseScale * animScale);
				break;
			}
			case AnimationPath::Weights:
				// Morph target weights not yet implemented
				break;
		}
	}
}

void AnimationComponent::FindKeyframes(const std::vector<float> &times, float time,
                                       size_t &outIndex0, size_t &outIndex1, float &outT) const
{
	if (times.empty())
	{
		outIndex0 = 0;
		outIndex1 = 0;
		outT      = 0.0f;
		return;
	}

	if (times.size() == 1 || time <= times.front())
	{
		outIndex0 = 0;
		outIndex1 = 0;
		outT      = 0.0f;
		return;
	}

	if (time >= times.back())
	{
		outIndex0 = times.size() - 1;
		outIndex1 = times.size() - 1;
		outT      = 0.0f;
		return;
	}

	// Binary search for the keyframe
	auto it = std::lower_bound(times.begin(), times.end(), time);
	if (it == times.begin())
	{
		outIndex0 = 0;
		outIndex1 = 0;
		outT      = 0.0f;
		return;
	}

	outIndex1 = static_cast<size_t>(std::distance(times.begin(), it));
	outIndex0 = outIndex1 - 1;

	float t0 = times[outIndex0];
	float t1 = times[outIndex1];
	float dt = t1 - t0;

	if (dt > 0.0f)
	{
		outT = (time - t0) / dt;
	}
	else
	{
		outT = 0.0f;
	}
}

glm::vec3 AnimationComponent::SampleVec3(const AnimationSampler &sampler, float time) const
{
	if (sampler.inputTimes.empty() || sampler.outputValues.size() < 3)
	{
		return glm::vec3(0.0f);
	}

	size_t index0, index1;
	float  t;
	FindKeyframes(sampler.inputTimes, time, index0, index1, t);

	// Get values at keyframes (3 floats per vec3)
	size_t offset0 = index0 * 3;
	size_t offset1 = index1 * 3;

	if (offset0 + 2 >= sampler.outputValues.size())
	{
		offset0 = sampler.outputValues.size() - 3;
	}
	if (offset1 + 2 >= sampler.outputValues.size())
	{
		offset1 = sampler.outputValues.size() - 3;
	}

	glm::vec3 v0(sampler.outputValues[offset0],
	             sampler.outputValues[offset0 + 1],
	             sampler.outputValues[offset0 + 2]);
	glm::vec3 v1(sampler.outputValues[offset1],
	             sampler.outputValues[offset1 + 1],
	             sampler.outputValues[offset1 + 2]);

	// Interpolate based on interpolation type
	switch (sampler.interpolation)
	{
		case AnimationInterpolation::Step:
			return v0;
		case AnimationInterpolation::Linear:
			return glm::mix(v0, v1, t);
		case AnimationInterpolation::CubicSpline:
			// For cubic spline, the output has in-tangent, value, out-tangent
			// Simplified: just use linear interpolation for now
			// Full cubic spline would require reading tangents from output data
			return glm::mix(v0, v1, t);
		default:
			return glm::mix(v0, v1, t);
	}
}

glm::quat AnimationComponent::SampleQuat(const AnimationSampler &sampler, float time) const
{
	if (sampler.inputTimes.empty() || sampler.outputValues.size() < 4)
	{
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}

	size_t index0, index1;
	float  t;
	FindKeyframes(sampler.inputTimes, time, index0, index1, t);

	// Get values at keyframes (4 floats per quaternion: x, y, z, w)
	size_t offset0 = index0 * 4;
	size_t offset1 = index1 * 4;

	if (offset0 + 3 >= sampler.outputValues.size())
	{
		offset0 = sampler.outputValues.size() - 4;
	}
	if (offset1 + 3 >= sampler.outputValues.size())
	{
		offset1 = sampler.outputValues.size() - 4;
	}

	// glTF quaternions are stored as (x, y, z, w)
	glm::quat q0(sampler.outputValues[offset0 + 3],         // w
	             sampler.outputValues[offset0],             // x
	             sampler.outputValues[offset0 + 1],         // y
	             sampler.outputValues[offset0 + 2]);        // z
	glm::quat q1(sampler.outputValues[offset1 + 3],         // w
	             sampler.outputValues[offset1],             // x
	             sampler.outputValues[offset1 + 1],         // y
	             sampler.outputValues[offset1 + 2]);        // z

	// Interpolate based on interpolation type
	switch (sampler.interpolation)
	{
		case AnimationInterpolation::Step:
			return q0;
		case AnimationInterpolation::Linear:
			return glm::slerp(q0, q1, t);
		case AnimationInterpolation::CubicSpline:
			// Simplified: use slerp for now
			return glm::slerp(q0, q1, t);
		default:
			return glm::slerp(q0, q1, t);
	}
}

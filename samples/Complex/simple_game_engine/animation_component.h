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
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>

#include "component.h"
#include "model_loader.h"

class Entity;
class TransformComponent;

/**
 * @brief Component that handles skeletal/transform animation playback.
 *
 * This component stores animation clips and plays them back by interpolating
 * keyframes and applying transforms to target nodes (entities).
 */
class AnimationComponent final : public Component
{
  public:
	/**
	 * @brief Constructor with optional name.
	 * @param componentName The name of the component.
	 */
	explicit AnimationComponent(const std::string &componentName = "AnimationComponent") :
	    Component(componentName)
	{}

	/**
	 * @brief Set the animations for this component.
	 * @param anims Vector of Animation clips to use.
	 */
	void SetAnimations(const std::vector<Animation> &anims)
	{
		animations = anims;
		if (!animations.empty())
		{
			currentAnimationIndex = 0;
		}
	}

	/**
	 * @brief Get the animations stored in this component.
	 * @return Reference to the animations vector.
	 */
	[[nodiscard]] const std::vector<Animation> &GetAnimations() const
	{
		return animations;
	}

	/**
	 * @brief Set the mapping from glTF node indices to entity pointers.
	 * This allows the animation system to apply transforms to the correct entities.
	 * @param mapping Map from node index to Entity pointer.
	 */
	void SetNodeToEntityMap(const std::unordered_map<int, Entity *> &mapping)
	{
		nodeToEntity = mapping;
	}

	/**
	 * @brief Play an animation by index.
	 * @param index The index of the animation to play.
	 * @param loop Whether to loop the animation (default: true).
	 */
	void Play(size_t index, bool loop = true)
	{
		if (index < animations.size())
		{
			currentAnimationIndex = static_cast<int>(index);
			currentTime           = 0.0f;
			playing               = true;
			looping               = loop;
		}
	}

	/**
	 * @brief Play an animation by name.
	 * @param name The name of the animation to play.
	 * @param loop Whether to loop the animation (default: true).
	 */
	void PlayByName(const std::string &name, bool loop = true)
	{
		for (size_t i = 0; i < animations.size(); ++i)
		{
			if (animations[i].name == name)
			{
				Play(i, loop);
				return;
			}
		}
	}

	/**
	 * @brief Stop the current animation.
	 */
	void Stop()
	{
		playing = false;
	}

	/**
	 * @brief Pause the current animation.
	 */
	void Pause()
	{
		playing = false;
	}

	/**
	 * @brief Resume a paused animation.
	 */
	void Resume()
	{
		playing = true;
	}

	/**
	 * @brief Check if an animation is currently playing.
	 * @return True if playing, false otherwise.
	 */
	[[nodiscard]] bool IsPlaying() const
	{
		return playing;
	}

	/**
	 * @brief Set the playback speed multiplier.
	 * @param speed The speed multiplier (1.0 = normal speed).
	 */
	void SetSpeed(float speed)
	{
		playbackSpeed = speed;
	}

	/**
	 * @brief Get the current playback speed.
	 * @return The playback speed multiplier.
	 */
	[[nodiscard]] float GetSpeed() const
	{
		return playbackSpeed;
	}

	/**
	 * @brief Get the current animation time.
	 * @return The current time in seconds.
	 */
	[[nodiscard]] float GetCurrentTime() const
	{
		return currentTime;
	}

	/**
	 * @brief Get the duration of the current animation.
	 * @return The duration in seconds, or 0 if no animation is selected.
	 */
	[[nodiscard]] float GetCurrentDuration() const
	{
		if (currentAnimationIndex >= 0 && currentAnimationIndex < static_cast<int>(animations.size()))
		{
			return animations[currentAnimationIndex].GetDuration();
		}
		return 0.0f;
	}

	/**
	 * @brief Update the animation, advancing time and applying transforms.
	 * @param deltaTime The time elapsed since the last update.
	 */
	void Update(std::chrono::milliseconds deltaTime) override;

  private:
	std::vector<Animation>            animations;
	std::unordered_map<int, Entity *> nodeToEntity;        // Maps glTF node index to Entity

	// Store base transforms for each animated node (captured when animation starts)
	// Animation transforms are applied relative to these base transforms
	std::unordered_map<int, glm::vec3> basePositions;
	std::unordered_map<int, glm::quat> baseRotations;        // Quaternions for proper rotation composition
	std::unordered_map<int, glm::vec3> baseScales;

	int   currentAnimationIndex = -1;
	float currentTime           = 0.0f;
	float playbackSpeed         = 1.0f;
	bool  playing               = false;
	bool  looping               = true;

	/**
	 * @brief Sample a vec3 value from a sampler at a given time.
	 * @param sampler The animation sampler.
	 * @param time The time to sample at.
	 * @return The interpolated vec3 value.
	 */
	[[nodiscard]] glm::vec3 SampleVec3(const AnimationSampler &sampler, float time) const;

	/**
	 * @brief Sample a quaternion value from a sampler at a given time.
	 * @param sampler The animation sampler.
	 * @param time The time to sample at.
	 * @return The interpolated quaternion value.
	 */
	[[nodiscard]] glm::quat SampleQuat(const AnimationSampler &sampler, float time) const;

	/**
	 * @brief Find the keyframe indices for interpolation.
	 * @param times The input time array.
	 * @param time The current time.
	 * @param outIndex0 Output: the lower keyframe index.
	 * @param outIndex1 Output: the upper keyframe index.
	 * @param outT Output: the interpolation factor (0-1).
	 */
	void FindKeyframes(const std::vector<float> &times, float time,
	                   size_t &outIndex0, size_t &outIndex1, float &outT) const;
};

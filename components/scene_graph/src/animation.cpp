/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include "animation.h"

#include "com/graph.hpp"

namespace vkb
{
namespace sg
{
void Animation::add_channel(Node &node, const AnimationTarget &target, const AnimationSampler &sampler)
{
	channels.push_back({node, target, sampler});
}

void Animation::update(float delta_time)
{
	current_time += delta_time;
	if (current_time > end_time)
	{
		current_time -= end_time;
	}

	for (auto &channel : channels)
	{
		for (size_t i = 0; i < channel.sampler.inputs.size() - 1; ++i)
		{
			if ((current_time >= channel.sampler.inputs[i]) && (current_time <= channel.sampler.inputs[i + 1]))
			{
				float time = (current_time - channel.sampler.inputs[i]) / (channel.sampler.inputs[i + 1] - channel.sampler.inputs[i]);

				auto &transform = channel.node.get_transform();

				if (channel.sampler.type == AnimationType::Linear)
				{
					switch (channel.target)
					{
						case Translation: {
							transform.set_translation(glm::vec3(glm::mix(channel.sampler.outputs[i],
							                                             channel.sampler.outputs[i + 1],
							                                             time)));
							break;
						}
						case Rotation: {
							glm::quat q1;
							q1.x = channel.sampler.outputs[i].x;
							q1.y = channel.sampler.outputs[i].y;
							q1.z = channel.sampler.outputs[i].z;
							q1.w = channel.sampler.outputs[i].w;

							glm::quat q2;
							q2.x = channel.sampler.outputs[i + 1].x;
							q2.y = channel.sampler.outputs[i + 1].y;
							q2.z = channel.sampler.outputs[i + 1].z;
							q2.w = channel.sampler.outputs[i + 1].w;

							transform.set_rotation(glm::normalize(glm::slerp(q1, q2, time)));
							break;
						}

						case Scale: {
							transform.set_scale(glm::vec3(glm::mix(channel.sampler.outputs[i],
							                                       channel.sampler.outputs[i + 1],
							                                       time)));
						}
					}
				}
				else if (channel.sampler.type == AnimationType::Step)
				{
					switch (channel.target)
					{
						case Translation: {
							transform.set_translation(glm::vec3(channel.sampler.outputs[i]));
							break;
						}
						case Rotation: {
							glm::quat q1;
							q1.x = channel.sampler.outputs[i].x;
							q1.y = channel.sampler.outputs[i].y;
							q1.z = channel.sampler.outputs[i].z;
							q1.w = channel.sampler.outputs[i].w;

							transform.set_rotation(glm::normalize(q1));
							break;
						}

						case Scale: {
							transform.set_scale(glm::vec3(channel.sampler.outputs[i]));
						}
					}
				}
				else if (channel.sampler.type == AnimationType::CubicSpline)
				{
					float delta = channel.sampler.inputs[i + 1] - channel.sampler.inputs[i];

					glm::vec4 p0 = channel.sampler.outputs[i * 3 + 1];              // Starting point
					glm::vec4 p1 = channel.sampler.outputs[(i + 1) * 3 + 1];        // Ending point

					glm::vec4 m0 = delta * channel.sampler.outputs[i * 3 + 2];              // Delta time * out tangent
					glm::vec4 m1 = delta * channel.sampler.outputs[(i + 1) * 3 + 0];        // Delta time * in tangent of next point

					// This equation is taken from the GLTF 2.0 specification Appendix C (https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation)
					glm::vec4 result = (2.0f * glm::pow(time, 3.0f) - 3.0f * glm::pow(time, 2.0f) + 1.0f) * p0 + (glm::pow(time, 3.0f) - 2.0f * glm::pow(time, 2.0f) + time) * m0 + (-2.0f * glm::pow(time, 3.0f) + 3.0f * glm::pow(time, 2.0f)) * p1 + (glm::pow(time, 3.0f) - glm::pow(time, 2.0f)) * m1;

					auto &transform = channel.node.get_transform();

					switch (channel.target)
					{
						case Translation: {
							transform.set_translation(glm::vec3(result));
							break;
						}
						case Rotation: {
							glm::quat q1;
							q1.x = result.x;
							q1.y = result.y;
							q1.z = result.z;
							q1.w = result.w;

							transform.set_rotation(glm::normalize(q1));
							break;
						}

						case Scale: {
							transform.set_scale(glm::vec3(result));
						}
					}
				}
			}
		}
	}
}

void Animation::update_times(float new_start_time, float new_end_time)
{
	if (new_start_time < start_time)
	{
		start_time = new_start_time;
	}
	if (new_end_time > end_time)
	{
		end_time = new_end_time;
	}
}

}        // namespace sg
}        // namespace vkb

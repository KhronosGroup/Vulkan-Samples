#pragma once

#include <functional>

#include "components/scene_graph/graph.hpp"

namespace components
{
namespace sg
{
using NodeAnimationFunc = std::function<void(float, Transform &)>;

struct NodeAnimation
{
	NodeAnimationFunc func;
};

namespace systems
{
/**
 * @brief Step forward all node animation functions
 * 
 * @param registry a scene graph registry
 * @param delta_time portion of time to step forward by
 */
void step_node_animation_funcs(Registry &registry, float delta_time);
}        // namespace systems

struct AnimationManagerInner
{
	std::string name;
	float       current_time{0.0f};
	float       start_time{std::numeric_limits<float>::max()};
	float       end_time{std::numeric_limits<float>::min()};
	bool        playing{false};
};

struct AnimationManager
{
	inline std::string name() const
	{
		return m_inner->name;
	}

	inline void play()
	{
		m_inner->playing = true;
	}

	inline void pause()
	{
		m_inner->playing = false;
	}

	inline void reset()
	{
		m_inner->current_time = 0.0f;
	}

	std::shared_ptr<AnimationManagerInner> m_inner;
};

enum AnimationType
{
	Linear,
	Step,
	CubicSpline
};

enum AnimationTarget
{
	Translation,
	Rotation,
	Scale
};

struct AnimationSampler
{
	AnimationType type{Linear};

	std::vector<float> inputs{};

	std::vector<glm::vec4> outputs{};
};

struct AnimationChannel
{
	std::weak_ptr<AnimationManagerInner> manager;

	std::weak_ptr<Node> node;

	AnimationTarget target;

	AnimationSampler sampler;
};

namespace systems
{
/**
 * @brief Step all animations forward by a given amount
 * 
 * @param registry a scene graph registry
 * @param delta_time portion of time to step forward by
 */
void step_animation(Registry &registry, float delta_time);
}        // namespace systems

}        // namespace sg
}        // namespace components

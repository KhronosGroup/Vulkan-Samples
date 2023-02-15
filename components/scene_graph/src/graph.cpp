#include "components/scene_graph/graph.hpp"

#include "components/scene_graph/components/transform.hpp"

namespace components
{
namespace sg
{

NodePtr Node::create(Registry &registry, const std::string &name, Transform transform)
{
	auto node = std::shared_ptr<Node>(new Node(registry, name));
	node->emplace_component<Transform>(transform);
	node->emplace_component<SceneNode>(std::weak_ptr<Node>{node});
	return node;
}

Node::~Node()
{
	if (m_parent)
	{
		m_parent->remove_child(this);
	}

	m_registry->destroy(m_entity);
}

const Node *Node::parent() const
{
	std::lock_guard<std::mutex> lock{m_mut};
	return m_parent;
}

Node *Node::parent()
{
	std::lock_guard<std::mutex> lock{m_mut};
	return m_parent;
}

void Node::remove_child(NodePtr &child)
{
	std::lock_guard<std::mutex> lock{m_mut};

	remove_child(child.get());
}

void Node::add_child(NodePtr &child)
{
	std::lock_guard<std::mutex> lock{m_mut};

	if (child->parent() == this)
	{
		return;
	}

	if (child->parent())
	{
		child->parent()->remove_child(child);
	}

	child->m_parent = this;
	m_children.emplace_back(child);
}

const std::vector<NodePtr> Node::children() const
{
	std::lock_guard<std::mutex> lock{m_mut};

	return {m_children.begin(), m_children.end()};
}

const std::vector<NodePtr> &Node::children()
{
	std::lock_guard<std::mutex> lock{m_mut};

	return m_children;
}

glm::mat4 Node::world_matrix() const
{
	glm::mat4 world_mat = get_component<Transform>().matrix();

	if (m_parent)
	{
		world_mat = m_parent->world_matrix() * world_mat;
	}

	return world_mat;
}

Node::Node(Registry &registry, const std::string &name) :
    name{name},
    m_entity{registry->create()},
    m_registry{registry}
{
}

void Node::remove_child(Node *child)
{
	for (size_t i = 0; i < m_children.size(); i++)
	{
		if (m_children[i].get() == child)
		{
			if (child->m_parent)
			{
				child->m_parent = nullptr;
			}

			m_children.erase(m_children.begin() + i);

			return;
		}
	}
}

}        // namespace sg
}        // namespace components
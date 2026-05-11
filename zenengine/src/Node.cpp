#include "pch.hpp"
#include "Node.hpp"

// ============================================================
// Node - Implementation
// ============================================================

// ----------------------------------------------------------
// Constructor / Destructor
// ----------------------------------------------------------

Node::Node(const std::string& p_name)
    : name(p_name)
    , m_parent(nullptr)
    , m_in_tree(false)
    , m_tree(nullptr)
    , m_children_draw_order_dirty(false)
    , m_queued_for_deletion(false)
{
}

Node::~Node()
{
    for (Node* child : m_children)
    {
        delete child;
    }
}

// ----------------------------------------------------------
// Virtual callbacks - default empty implementations
// ----------------------------------------------------------

void Node::_ready()
{
}

void Node::_update(float /*dt*/)
{
}

void Node::_draw()
{
}

void Node::_on_destroy()
{
}

int Node::get_draw_order() const
{
    return 0;
}

// ----------------------------------------------------------
// Child management
// ----------------------------------------------------------

void Node::add_child(Node* child)
{
    assert(child != nullptr && "add_child: null pointer");
    assert(child != this    && "add_child: cannot add self as child");
    assert(!child->m_parent && "add_child: node already has a parent");

    child->m_parent = this;
    m_children.push_back(child);
    mark_children_draw_order_dirty();

    if (m_in_tree)
    {
        child->set_tree_recursive_internal(m_tree);
        child->_do_enter_tree();
        child->_do_ready();
    }
}

Node* Node::remove_child(Node* child)
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        if (*it == child)
        {
            if (m_in_tree)
            {
                child->_do_exit_tree();
            }

            child->set_tree_recursive_internal(nullptr);
            child->m_parent = nullptr;
            m_children.erase(it);
            mark_children_draw_order_dirty();
            return child;
        }
    }

    return nullptr;
}

void Node::destroy_child(Node* child)
{
    delete remove_child(child);
}

// ----------------------------------------------------------
// Tree access
// ----------------------------------------------------------

Node* Node::get_parent() const
{
    return m_parent;
}

bool Node::is_in_tree() const
{
    return m_in_tree;
}

size_t Node::get_child_count() const
{
    return m_children.size();
}

Node* Node::get_child(size_t index) const
{
    if (index >= m_children.size())
    {
        return nullptr;
    }

    return m_children[index];
}

Node* Node::find_child(const std::string& child_name) const
{
    for (Node* child : m_children)
    {
        if (child->name == child_name)
        {
            return child;
        }
    }

    return nullptr;
}

Node* Node::find_node(const std::string& node_name) const
{
    if (name == node_name)
    {
        return const_cast<Node*>(this);
    }

    for (Node* child : m_children)
    {
        Node* found = child->find_node(node_name);

        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

std::string Node::get_path() const
{
    if (!m_parent)
    {
        return name;
    }

    return m_parent->get_path() + "/" + name;
}

Node* Node::get_root() const
{
    const Node* current = this;

    while (current->m_parent)
    {
        current = current->m_parent;
    }

    return const_cast<Node*>(current);
}

SceneTree* Node::get_tree() const
{
    return m_tree;
}

// ----------------------------------------------------------
// Game loop propagation
// ----------------------------------------------------------

void Node::propagate_update(float dt)
{
    if (!active)
    {
        return;
    }

    _update(dt);

    for (Node* child : m_children)
    {
        child->propagate_update(dt);
    }
}

void Node::propagate_draw()
{
    if (!visible)
    {
        return;
    }

    _draw();

    if (m_children_draw_order_dirty)
    {
        sort_children_for_draw();
    }

    for (Node* child : m_children)
    {
        child->propagate_draw();
    }
}

void Node::mark_children_draw_order_dirty()
{
    m_children_draw_order_dirty = true;
}

void Node::queue_free()
{
    m_queued_for_deletion = true;
}

bool Node::is_queued_for_deletion() const
{
    return m_queued_for_deletion;
}

void Node::sort_children_for_draw()
{
    std::stable_sort(m_children.begin(), m_children.end(),
        [](const Node* a, const Node* b)
        {
            return a->get_draw_order() < b->get_draw_order();
        });
    m_children_draw_order_dirty = false;
}

void Node::set_tree_recursive_internal(SceneTree* tree)
{
    m_tree = tree;

    for (Node* child : m_children)
    {
        if (child)
        {
            child->set_tree_recursive_internal(tree);
        }
    }
}

// ----------------------------------------------------------
// Private - internal tree notifications
// ----------------------------------------------------------

void Node::_do_enter_tree()
{
    m_in_tree = true;

    for (Node* child : m_children)
    {
        child->_do_enter_tree();
    }
}

void Node::_do_ready()
{
    for (Node* child : m_children)
    {
        child->_do_ready();
    }

    _ready();
}

void Node::_do_exit_tree()
{
    for (Node* child : m_children)
    {
        child->_do_exit_tree();
    }

    _on_destroy();
    m_in_tree = false;
}

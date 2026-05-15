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
    , m_script_host(nullptr)
{
}

Node::~Node()
{
    delete m_script_host;
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
    if (m_script_host && m_script_host->has_method("_ready"))
        m_script_host->on_ready(this);
}

void Node::_update(float dt)
{
    if (m_script_host && m_script_host->has_method("_update"))
        m_script_host->on_update(this, dt);
}

void Node::_draw()
{
    if (m_script_host && m_script_host->has_method("_draw"))
        m_script_host->on_draw(this);
}

void Node::_on_destroy()
{
    if (m_script_host && m_script_host->has_method("_on_destroy"))
        m_script_host->on_destroy(this);
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

// Godot-style path: "Child", "../Sibling", "Child/Grandchild", "/root/Name"
Node* Node::get_node(const std::string& path) const
{
    if (path.empty()) return const_cast<Node*>(this);

    // Absolute path starting with '/'
    if (path[0] == '/')
    {
        const Node* root = get_root();
        // Strip leading '/'
        std::string rel = path.substr(1);
        // Strip optional "root/" prefix to match Godot convention
        if (rel.rfind("root/", 0) == 0) rel = rel.substr(5);
        return root->get_node(rel);
    }

    // Walk segments separated by '/'
    const Node* current = this;
    std::string rest = path;
    while (!rest.empty() && current)
    {
        std::string seg;
        auto slash = rest.find('/');
        if (slash == std::string::npos)
        {
            seg  = rest;
            rest = "";
        }
        else
        {
            seg  = rest.substr(0, slash);
            rest = rest.substr(slash + 1);
        }

        if (seg == ".")
        {
            // stay
        }
        else if (seg == "..")
        {
            current = current->m_parent;
        }
        else
        {
            current = current->find_child(seg);
        }
    }
    return const_cast<Node*>(current);
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

// ----------------------------------------------------------
// Signals
// ----------------------------------------------------------

void Node::connect(const std::string& signal, SignalCallback cb, void* tag)
{
    m_signals[signal].push_back({std::move(cb), tag});
}

void Node::disconnect(const std::string& signal, void* tag)
{
    auto it = m_signals.find(signal);
    if (it == m_signals.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [tag](const SignalConnection& c){ return c.tag == tag; }), vec.end());
}

void Node::disconnect_all(void* tag)
{
    for (auto& [sig, vec] : m_signals)
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [tag](const SignalConnection& c){ return c.tag == tag; }), vec.end());
}

void Node::emit_signal(const std::string& signal, const SignalArgs& args)
{
    auto it = m_signals.find(signal);
    if (it != m_signals.end())
    {
        // Copy list — a callback may connect/disconnect during emit
        std::vector<SignalConnection> snapshot = it->second;
        for (const auto& conn : snapshot)
            conn.callback(args);
    }
    // Let the script host handle signals connected from script
    if (m_script_host)
        m_script_host->on_signal(this, signal.c_str(), args);
}

bool Node::has_connections(const std::string& signal) const
{
    auto it = m_signals.find(signal);
    return it != m_signals.end() && !it->second.empty();
}

// ----------------------------------------------------------
// Script host
// ----------------------------------------------------------

void Node::set_script_host(ScriptHost* host)
{
    delete m_script_host;
    m_script_host = host;
}

// ----------------------------------------------------------
// Groups
// ----------------------------------------------------------

void Node::add_to_group(const std::string& group)
{
    for (const auto& g : m_groups)
        if (g == group) return;  // already in group
    m_groups.push_back(group);
}

void Node::remove_from_group(const std::string& group)
{
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it)
    {
        if (*it == group) { m_groups.erase(it); return; }
    }
}

bool Node::is_in_group(const std::string& group) const
{
    for (const auto& g : m_groups)
        if (g == group) return true;
    return false;
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

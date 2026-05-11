#pragma once

#include "pch.hpp"

// Forward declaration
class SceneTree;

// ============================================================
// Node - Base class for all MiniGodot2D nodes.
//
// Ownership rules:
//   - Parent owns its children (destructor deletes them in cascade)
//   - SceneTree owns the root node
//   - Never use smart pointers for nodes
//
// Lifecycle (mirrors Godot behaviour):
//   _ready()      - post-order : children ready before parent
//   _update(dt)   - top-down   : parent updates before children
//   _draw()       - top-down   : parent draws before children
//   _on_destroy() - bottom-up  : children destroyed before parent
// ============================================================

class Node
{
public:

    std::string name;
    bool        visible = true;
    bool        active  = true;

    // ----------------------------------------------------------
    // Constructor / Destructor
    // ----------------------------------------------------------

    explicit Node(const std::string& p_name = "Node");

    // Cascade destructor - parent deletes all children
    virtual ~Node();

    // No copy - nodes are not copyable
    Node(const Node&)            = delete;
    Node& operator=(const Node&) = delete;

    // ----------------------------------------------------------
    // Virtual callbacks - override in subclasses
    // ----------------------------------------------------------

    virtual void _ready();
    virtual void _update(float dt);
    virtual void _draw();
    virtual void _on_destroy();
    virtual int  get_draw_order() const;

    // ----------------------------------------------------------
    // Child management
    // ----------------------------------------------------------

    // Add child - transfers ownership to this node
    void  add_child(Node* child);

    // Remove child WITHOUT destroying - caller takes ownership
    Node* remove_child(Node* child);

    // Remove child AND destroy it immediately
    void  destroy_child(Node* child);

    // ----------------------------------------------------------
    // Tree access
    // ----------------------------------------------------------

    Node*       get_parent()      const;
    bool        is_in_tree()      const;
    size_t      get_child_count() const;
    Node*       get_child(size_t index) const;

    // Find direct child by name (non-recursive)
    Node*       find_child(const std::string& child_name) const;

    // Find any node by name in subtree (recursive)
    Node*       find_node(const std::string& node_name) const;

    // Returns path from root: "Root/Player/Sprite"
    std::string get_path() const;

    // Returns the root of the tree
    Node*       get_root() const;
    SceneTree*  get_tree() const;
    void mark_children_draw_order_dirty();
    void queue_free();
    bool is_queued_for_deletion() const;

    // ----------------------------------------------------------
    // Game loop propagation - called by SceneTree, not subclasses
    // ----------------------------------------------------------

    virtual void propagate_update(float dt);
    virtual void propagate_draw();

protected:
    Node*              m_parent;
    std::vector<Node*> m_children;
    bool               m_in_tree;
    SceneTree*         m_tree;
    bool               m_children_draw_order_dirty;
    bool               m_queued_for_deletion;

private:
    void sort_children_for_draw();
    void set_tree_recursive_internal(SceneTree* tree);

    void _do_enter_tree();
    void _do_ready();
    void _do_exit_tree();

    friend class SceneTree;
};

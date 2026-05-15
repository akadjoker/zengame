#pragma once

#include "pch.hpp"
#include "ScriptHost.hpp"
#include "Signal.hpp"

// Forward declarations
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

// Type tag used to avoid dynamic_cast in hot paths.
// Each concrete class sets its own tag in its constructor.
enum class NodeType : uint8_t
{
    Node                = 0,
    Node2D              = 1,
    // ── Non-collision Node2D subtypes ──
    Sprite2D            = 2,
    AnimatedSprite2D    = 3,
    TileMap2D           = 4,
    View2D              = 5,
    ParticleEmitter2D   = 6,
    Parallax2D          = 7,
    ParallaxLayer2D     = 8,
    ShadowCaster2D      = 9,
    PolyMesh2D          = 10,
    TextNode2D          = 11,
    NavigationGrid2D    = 12,
    RayCast2D           = 13,
    // ── Collision types (must be contiguous) ──
    Collider2D          = 14,
    CollisionObject2D   = 15,
    CharacterBody2D     = 16,
    StaticBody2D        = 17,
    Area2D              = 18,
    RigidBody2D         = 19,
    // ── Non-collision Node2D extras ──
    Path2D              = 20,
    PathFollow2D        = 21,
    Line2D              = 22,
    Rect2D              = 23,
    Circle2D            = 24,
    Bone2D              = 25,
    Skeleton2D          = 26,
    MotionStreak2D      = 27,
    DrawNode2D          = 28,
};

class Node
{
public:

    std::string name;
    bool        visible = true;
    bool        active  = true;
    NodeType    node_type = NodeType::Node;

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

    // Godot-style path navigation: "Child", "../Sibling", "/root/Player"
    Node*       get_node(const std::string& path) const;

    // Returns path from root: "Root/Player/Sprite"
    std::string get_path() const;

    // Returns the root of the tree
    Node*       get_root() const;
    SceneTree*  get_tree() const;
    void mark_children_draw_order_dirty();
    void queue_free();
    bool is_queued_for_deletion() const;

    // ----------------------------------------------------------
    // Groups — like Godot add_to_group / is_in_group
    // ----------------------------------------------------------
    void add_to_group   (const std::string& group);
    void remove_from_group(const std::string& group);
    bool is_in_group    (const std::string& group) const;

    // ----------------------------------------------------------
    // Signals — Godot-style decoupled communication
    // ----------------------------------------------------------

    // Connect a signal by name to a callback.
    // tag = optional identifier for later disconnection (usually `this` of the listener).
    void connect   (const std::string& signal, SignalCallback cb, void* tag = nullptr);

    // Remove all connections for this signal that have the given tag.
    void disconnect(const std::string& signal, void* tag);

    // Remove ALL connections to every signal that have the given tag.
    // Call this from the listener's _on_destroy() to avoid dangling callbacks.
    void disconnect_all(void* tag);

    // Fire a signal — calls all connected callbacks in order.
    void emit_signal(const std::string& signal, const SignalArgs& args = {});

    bool has_connections(const std::string& signal) const;
    void        set_script_host(ScriptHost* host);  // engine takes ownership
    ScriptHost* get_script_host() const { return m_script_host; }    // Fast type checks — avoids dynamic_cast in hot paths
    bool is_a(NodeType t)   const { return node_type == t; }
    bool is_node2d()        const { return static_cast<uint8_t>(node_type) >= static_cast<uint8_t>(NodeType::Node2D); }
    bool is_collision_obj() const { return static_cast<uint8_t>(node_type) >= static_cast<uint8_t>(NodeType::CollisionObject2D)
                                        && static_cast<uint8_t>(node_type) <= static_cast<uint8_t>(NodeType::RigidBody2D); }

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
    ScriptHost*        m_script_host = nullptr;
    std::vector<std::string> m_groups;
    std::unordered_map<std::string, std::vector<SignalConnection>> m_signals;

private:
    void sort_children_for_draw();
    void set_tree_recursive_internal(SceneTree* tree);

    void _do_enter_tree();
    void _do_ready();
    void _do_exit_tree();

    friend class SceneTree;
};

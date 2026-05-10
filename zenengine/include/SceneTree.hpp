#pragma once

#include "pch.hpp"
#include "Node.hpp"
#include "math.hpp"
#include "SpatialHash2D.hpp"
class View2D;
class CollisionObject2D;
class Renderer2D;

// ============================================================
// SceneTree - Manages the node tree and the main game loop.
//
// Ownership:
//   - SceneTree owns the root node
//   - Destroying SceneTree destroys the entire tree in cascade
//
// Usage:
//   SceneTree tree;
//   tree.set_root(new MyScene("Game"));
//   tree.start();
//   while (tree.is_running()) {
//       tree.process(dt);
//       tree.draw();
//   }
// ============================================================

class SceneTree
{
public:
    enum DebugDrawFlags : uint32_t
    {
        DEBUG_NONE      = 0,
        DEBUG_PIVOT     = 1u << 0,
        DEBUG_DIRECTION = 1u << 1,
        DEBUG_SHAPES    = 1u << 2,
        DEBUG_NAMES     = 1u << 3,
        DEBUG_TREE      = 1u << 4,
        DEBUG_CONTACTS  = 1u << 5,
        DEBUG_SPATIAL   = 1u << 6
    };

    // ----------------------------------------------------------
    // Constructor / Destructor
    // ----------------------------------------------------------

    SceneTree();
    ~SceneTree();

    // No copy allowed
    SceneTree(const SceneTree&)            = delete;
    SceneTree& operator=(const SceneTree&) = delete;

    // ----------------------------------------------------------
    // Scene management
    // ----------------------------------------------------------

    // Set root node - SceneTree takes ownership
    void  set_root(Node* root);
    Node* get_root() const;

    // Optional UI root drawn after world, without camera transform.
    void  set_ui_root(Node* root);
    Node* get_ui_root() const;

    // ----------------------------------------------------------
    // Game loop
    // ----------------------------------------------------------

    void process(float dt);
    void draw();
    void start();
    void quit();
    bool is_running() const;
    View2D* get_current_camera() const;
    void query_collision_candidates(const Rectangle& area,
                                    const CollisionObject2D* self,
                                    std::vector<CollisionObject2D*>& out_candidates) const;
    void set_debug_draw_flags(uint32_t flags);
    uint32_t get_debug_draw_flags() const;
    bool has_debug_draw_flag(DebugDrawFlags flag) const;

private:
    struct DebugContact
    {
        Vec2 point;
        Vec2 normal;
        float depth = 0.0f;
        CollisionObject2D* a = nullptr;
        CollisionObject2D* b = nullptr;
    };

    void update_physics();
    void draw_debug(Node* node) const;
    void set_tree_recursive(Node* node, SceneTree* tree);
    void draw_tree_overlay() const;
    void collect_tree_lines(const Node* node, int depth, std::vector<std::string>& out_lines) const;
    void cleanup_queued_nodes(Node* node);
    void draw_world_pass();
    void draw_ui_pass();
    void draw_overlay_pass();

    Node* m_root;
    Node* m_ui_root;
    bool  m_running;
    uint32_t m_debug_draw_flags;
    SpatialHash2D m_physics_hash;
    std::unordered_map<int, CollisionObject2D*> m_physics_by_id;
    std::vector<DebugContact> m_debug_contacts;
    std::unordered_map<uint64_t, std::pair<class CollisionObject2D*, class CollisionObject2D*>> m_prev_collisions;

    friend class Renderer2D;
};

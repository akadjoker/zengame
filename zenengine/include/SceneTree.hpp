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
    // Node factory  — never use new directly
    // ----------------------------------------------------------

    // Create a node of type T. If name is empty, T's own default name is used.
    // SceneTree does NOT own the node until it is added to the tree.
    // Example:
    //   auto* sprite = tree.create<Sprite2D>("Player");
    //   root->add_child(sprite);
    template<typename T>
    T* create(const std::string& name = "")
    {
        return name.empty() ? new T() : new T(name);
    }

    // Create a node and immediately add it as a child of parent.
    // Returns the newly created node (already parented).
    // Example:
    //   auto* label = tree.create<TextNode2D>(root, "HUD_Label");
    template<typename T>
    T* create(Node* parent, const std::string& name = "")
    {
        T* node = name.empty() ? new T() : new T(name);
        if (parent) parent->add_child(node);
        return node;
    }

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

    // Load all tile layers from a .tmx file and add them as children of parent.
    // Each layer becomes a separate TileMap2D child node.
    // Returns the number of layers created, or -1 on parse error.
    // Never throws — errors are logged via TraceLog.
    int load_tilemap_from_tmx(const char* tmx_path, Node* parent);

    // Parse all <objectgroup> elements from a .tmx file.
    // Creates one StaticBody2D + Collider2D per object and adds them to parent.
    // Supports rectangle, ellipse and polygon objects.
    // Returns the number of collision bodies created, or -1 on parse error.
    int load_collision_from_tmx(const char* tmx_path, Node* parent);

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

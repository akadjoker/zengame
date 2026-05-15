#pragma once

#include "pch.hpp"
#include "Node.hpp"
#include "math.hpp"
#include "SpatialHash2D.hpp"
#include <functional>
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

    // Destroy all nodes immediately (call before unloading GPU assets).
    void  clean();

    // Optional UI root drawn after world, without camera transform.
    void  set_ui_root(Node* root);
    Node* get_ui_root() const;

    // Schedule a root swap to happen at the start of the next frame.
    // Constructs a node of type T, forwarding all arguments to its constructor.
    // Safe to call from inside _update / callbacks / timers.
    //
    // Example:
    //   tree.change_scene<GameScene>();
    //   tree.change_scene<Level>("level2.tmx", difficulty);
    template<typename T, typename... Args>
    void change_scene(Args&&... args)
    {
        m_pending_scene = [args...]() -> Node* { return new T(args...); };
    }

    // Like change_scene<T> but fades to black, swaps the scene, then fades back.
    // duration: seconds for each half (fade-out + fade-in).
    // fade_color: the overlay color (default black).
    //
    // Example:
    //   tree.change_scene_fade<GameScene>(0.4f);
    //   tree.change_scene_fade<Level>(0.5f, BLACK, "level2.tmx");
    template<typename T, typename... Args>
    void change_scene_fade(float duration, Color fade_color, Args&&... args)
    {
        m_fade_color    = fade_color;
        m_fade_duration = duration > 0.0f ? duration : 0.3f;
        m_fade_t        = 0.0f;
        m_fade_state    = FadeState::Out;
        m_pending_scene = [args...]() -> Node* { return new T(args...); };
    }

    // Convenience overload — fade to black with given duration.
    template<typename T, typename... Args>
    void change_scene_fade(float duration, Args&&... args)
    {
        change_scene_fade<T>(duration, BLACK, std::forward<Args>(args)...);
    }

    // Advanced: provide a custom factory lambda if you need extra setup
    // before _ready() fires (e.g. injecting dependencies, loading saves).
    //
    // Example:
    //   tree.change_scene_factory([]{ auto* s = new Game(); s->load("save.dat"); return s; });
    void change_scene_factory(std::function<Node*()> factory);

    // True if a scene change is pending (not yet applied).
    bool is_scene_change_pending() const { return (bool)m_pending_scene; }

    // ----------------------------------------------------------
    // Game loop
    // ----------------------------------------------------------

    void process(float dt);
    void draw();
    void start();
    void quit();
    bool is_running() const;
    View2D* get_current_camera() const;

    // ── Groups ────────────────────────────────────────────────────────────────
    // Collect all nodes in the tree that belong to a group.
    void get_nodes_in_group(const std::string& group, std::vector<Node*>& out) const;

    // Call a method on every node in a group (if it has a script with that method).
    // Extra args are passed as raw pointer — the ScriptHost implementation decides
    // how to marshal them (e.g. zenpy Value array).
    void call_group(const std::string& group, const char* method);

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

    std::function<Node*()> m_pending_scene;  // deferred scene change

    // ── Fade transition ───────────────────────────────────────────────────────
    enum class FadeState : uint8_t { None = 0, Out, In };
    FadeState m_fade_state    = FadeState::None;
    float     m_fade_t        = 0.0f;   // [0..1] progress within current half
    float     m_fade_duration = 0.3f;   // seconds per half
    Color     m_fade_color    = BLACK;

    void update_fade(float dt);
    void draw_fade_overlay() const;

    friend class Renderer2D;
};

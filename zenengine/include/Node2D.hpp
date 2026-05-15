
#pragma once

#include "pch.hpp"
#include "Node.hpp"
#include "math.hpp"

// ============================================================
// Node2D - Base class for all 2D nodes.
//
// Adds a 2D transform (position, rotation, scale, pivot) on top
// of Node. The global transform is cached and recomputed lazily
// using a per-frame revision counter to avoid redundant work when
// multiple systems query the same node in one frame.
//
// All angles are in DEGREES (converted internally to radians).
// ============================================================

class Node2D : public Node
{
public:

    // ----------------------------------------------------------
    // Constructor
    // ----------------------------------------------------------

    explicit Node2D(const std::string& p_name = "Node2D");

    // ----------------------------------------------------------
    // Local transform properties
    // ----------------------------------------------------------

    Vec2  position;    // local position relative to parent
    Vec2  scale;       // local scale  (1,1 = no scale)
    Vec2  pivot;       // pivot point for rotation and scale
    Vec2  skew;        // local skew (0,0 = no skew)
    float rotation;    // local rotation in DEGREES
    int   z_index;     // draw order relative to siblings

    // ----------------------------------------------------------
    // Transform helpers
    // ----------------------------------------------------------

    // Returns the local transform matrix (position + rotation + scale + pivot)
    Matrix2D get_local_transform() const;

    // Returns the world-space transform.
    // Result is cached per-frame; mark dirty with invalidate_transform().
    Matrix2D get_global_transform() const;

    // Returns the world-space position of this node
    Vec2 get_global_position() const;
    float get_global_rotation() const;

    Vec2 local_to_world(const Vec2& local_point) const;
    Vec2 world_to_local(const Vec2& world_point) const;

    // Invalidate the cached world transform for this node and all its children.
    // Call this when you modify position/rotation/scale/pivot manually.
    void invalidate_transform();

    // Move by delta in local space
    void translate(const Vec2& delta);

    // Rotate by delta degrees
    void rotate_by(float degrees);

    void advance(float dt);
    void advance(float dt, float angle);

    // Point local origin toward a world-space target
    void look_at(const Vec2& target);

    // ── Helpers ───────────────────────────────────────────────────────────────

    // Distance from world position of this node to a point / another node.
    float distance_to(const Vec2& world_point) const;
    float distance_to(const Node2D* other)     const;

    // Normalized direction from this node's world position toward target.
    Vec2  direction_to(const Vec2& world_point) const;
    Vec2  direction_to(const Node2D* other)     const;

    // Move position toward target by at most max_distance pixels.
    // Does NOT overshoot. Returns the new position (also sets position).
    Vec2  move_toward(const Vec2& target, float max_distance);

    // Rotate current rotation toward target_deg by at most max_delta degrees.
    // Handles wrap-around. Returns new rotation (also sets rotation).
    float rotate_toward(float target_deg, float max_delta);

    // Set position in world space (inverse-transforms through parent).
    void set_global_position(const Vec2& world_pos);

    // Convert a local-space point to world space
    Vec2 to_global(const Vec2& local_point) const;

    // Convert a world-space point to local space
    Vec2 to_local(const Vec2& world_point) const;

    void set_z_index(int value);
    int get_z_index() const;
    int get_draw_order() const override;

    // ── Immediate-mode draw helpers (call inside _draw only) ──────────────────
    // All positions/sizes are in LOCAL space — transformed to world automatically.

    // Filled or outlined circle. radius is in local-space pixels.
    void draw_circle(Vec2 local_pos, float radius, Color c, bool filled = true) const;

    // Filled or outlined axis-aligned rect. origin = top-left corner, in local space.
    void draw_rect(Vec2 local_origin, Vec2 size, Color c, bool filled = true) const;

    // Single line segment between two local-space points.
    void draw_line(Vec2 local_a, Vec2 local_b, Color c, float width = 1.0f) const;

    // Filled or outlined triangle.
    void draw_triangle(Vec2 a, Vec2 b, Vec2 c_pt, Color c, bool filled = true) const;

    // Called by SceneTree at the start of each frame to reset the cache.
    // Do not call manually.
    static void begin_frame(uint32_t frame_id);

private:

    mutable Matrix2D m_cached_world;
    mutable uint32_t m_cache_frame = 0xFFFFFFFFu;   // frame this cache is valid for

    static uint32_t  s_current_frame;
};

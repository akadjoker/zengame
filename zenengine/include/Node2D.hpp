
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

    // Convert a local-space point to world space
    Vec2 to_global(const Vec2& local_point) const;

    // Convert a world-space point to local space
    Vec2 to_local(const Vec2& world_point) const;

    void set_z_index(int value);
    int get_z_index() const;
    int get_draw_order() const override;

    // Called by SceneTree at the start of each frame to reset the cache.
    // Do not call manually.
    static void begin_frame(uint32_t frame_id);

private:

    mutable Matrix2D m_cached_world;
    mutable uint32_t m_cache_frame = 0xFFFFFFFFu;   // frame this cache is valid for

    static uint32_t  s_current_frame;
};

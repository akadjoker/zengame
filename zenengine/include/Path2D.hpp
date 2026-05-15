#pragma once

#include "Node2D.hpp"
#include <vector>
#include <raylib.h>

// ----------------------------------------------------------------------------
// Path2D
//
// A 2D Catmull-Rom spline defined by control points in local space.
// Used with PathFollow2D to move nodes along the curve at a given speed,
// or queried directly for world-space positions and directions.
//
// Usage:
//   auto* path = tree.create<Path2D>(root, "PatrolPath");
//   path->add_point({100, 500});
//   path->add_point({400, 300});
//   path->add_point({700, 500});
//   path->loop       = true;
//   path->show_debug = true;
//
//   auto* follower = tree.create<PathFollow2D>(path, "Enemy");
//   follower->speed  = 150.0f;
//   follower->rotate = true;
// ----------------------------------------------------------------------------
class Path2D : public Node2D
{
public:
    explicit Path2D(const std::string& p_name = "Path2D");

    // ── Control points (local space) ──────────────────────────────────────────

    std::vector<Vec2> points;

    // Connect the last point back to the first to form a closed loop.
    bool  loop        = false;

    // ── Debug ────────────────────────────────────────────────────────────────
    bool  show_debug  = false;
    Color debug_color = {80, 200, 80, 255};
    float debug_width = 2.0f;

    // ── Point management ─────────────────────────────────────────────────────

    // Append or insert a point.  at_index = -1 means append.
    void add_point   (Vec2 local_pos, int at_index = -1);
    void set_point   (int  index,     Vec2 local_pos);
    void remove_point(int  index);
    void clear_points();
    int  get_point_count() const { return (int)points.size(); }

    // ── Arc-length queries ────────────────────────────────────────────────────

    // Total path length in local-space units.
    float get_total_length() const;

    // World-space position at `dist` pixels along the path.
    Vec2  get_point_at_distance  (float dist) const;

    // World-space normalised tangent at `dist` pixels along the path.
    Vec2  get_direction_at_distance(float dist) const;

    // Closest offset (local-space pixels) to a world-space point.
    float get_closest_offset(Vec2 world_pos) const;

    // Force rebuild of the internal arc-length LUT.
    // Called automatically on the first query after the points change.
    void  bake() const;
    bool  needs_bake() const { return m_dirty; }

    // ── Node2D override ───────────────────────────────────────────────────────
    void _draw() override;

private:
    struct BakeEntry { float arc; float t; };

    mutable std::vector<BakeEntry> m_lut;
    mutable float                  m_total = 0.0f;
    mutable bool                   m_dirty = true;

    static constexpr int BAKE_STEPS = 12;   // subdivisions per segment

    void  ensure_baked()     const;
    Vec2  sample_local(float t) const;      // t in [0..N] (loop) or [0..N-1]
    float t_from_dist (float dist) const;   // arc-length → spline parameter
};

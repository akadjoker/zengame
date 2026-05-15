#pragma once

#include "Node2D.hpp"
#include <functional>

class Path2D;

// ----------------------------------------------------------------------------
// PathFollow2D
//
// Must be a direct child of a Path2D node.
// Each frame it advances `offset` by `speed * dt` and sets its own position
// (and optionally rotation) to the corresponding point on the parent path.
//
// Manual control (speed = 0):  just set `offset` directly.
// Normalised control:          use get_progress() / set_progress(0..1).
//
// Usage:
//   auto* path   = tree.create<Path2D>(root, "Rail");
//   path->add_point(...);
//   path->loop = true;
//
//   auto* plat   = tree.create<PathFollow2D>(path, "Platform");
//   plat->speed  = 120.0f;
//   plat->rotate = false;
//
//   auto* bullet = tree.create<PathFollow2D>(path, "Bullet");
//   bullet->speed            = 400.0f;
//   bullet->loop             = false;
//   bullet->on_reached_end   = [&]{ bullet->queue_free(); };
// ----------------------------------------------------------------------------
class PathFollow2D : public Node2D
{
public:
    explicit PathFollow2D(const std::string& p_name = "PathFollow2D");

    // ── Properties ────────────────────────────────────────────────────────────

    float offset    = 0.0f;    // current distance along path (local units)
    float speed     = 0.0f;    // pixels/second; 0 = manual via offset
    bool  loop      = true;    // wrap around at end / beginning
    bool  rotate    = true;    // align rotation to path tangent

    // Lateral offset (perpendicular to path direction).
    // Useful for parallel lanes, hovering entities, etc.
    float h_offset  = 0.0f;

    // ── Normalised progress [0..1] ────────────────────────────────────────────
    float get_progress() const;
    void  set_progress(float t);   // sets offset = t * total_length

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void()> on_loop_completed;   // fired every time the path wraps
    std::function<void()> on_reached_end;      // fired once when loop = false and end is hit

    // ── Node2D override ───────────────────────────────────────────────────────
    void _update(float dt) override;

private:
    Path2D* get_path() const;
};

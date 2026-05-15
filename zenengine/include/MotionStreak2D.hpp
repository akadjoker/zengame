#pragma once

#include "Node2D.hpp"
#include <vector>
#include <raylib.h>

// ----------------------------------------------------------------------------
// MotionStreak2D — textured ribbon trail that follows a moving node.
//
// Each frame, if the node moved more than min_seg pixels, a new point is added
// to the internal trail. Points older than time_to_fade seconds are removed.
// The ribbon is drawn as a strip of quads — each quad spans two consecutive
// points, with half-width perpendicular to the travel direction.
//
// Usage:
//   auto* streak = tree.create<MotionStreak2D>(parent, "Trail");
//   streak->time_to_fade = 0.5f;
//   streak->stroke_width = 20.0f;
//   streak->color        = {255, 200, 50, 255};
//
// The node's world position drives the streak head every frame.
// You do NOT need to set position manually — inherit it from the parent node
// or set position directly to move the streak emitter.
// ----------------------------------------------------------------------------

class MotionStreak2D : public Node2D
{
public:
    explicit MotionStreak2D(const std::string& p_name = "MotionStreak2D");

    // ── Parameters ───────────────────────────────────────────────────────────

    float time_to_fade  = 0.6f;   // seconds until each point fully fades
    float stroke_width  = 16.0f;  // ribbon width in pixels
    float min_seg       = 8.0f;   // minimum distance before adding a new point
    Color color         = WHITE;  // tint (alpha is overridden by age fade)

    // Optional texture (0 = solid colour ribbon)
    Texture2D texture = {0};

    // ── Control ──────────────────────────────────────────────────────────────

    // Clear all stored points immediately
    void reset();

    // Tint all living points to a new colour (keeps their individual alphas)
    void tint(Color new_color);

    // ── Node2D overrides ─────────────────────────────────────────────────────

    void _update(float dt) override;
    void _draw()           override;

    // Exposed so the free helper in .cpp can access it
    struct Point {
        Vec2  pos;
        float age;     // seconds since this point was created
    };

private:
    std::vector<Point> _points;
    Vec2               _last_pos = {0.f, 0.f};
    bool               _initialized = false;
};

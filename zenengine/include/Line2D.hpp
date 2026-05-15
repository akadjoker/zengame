#pragma once

#include "Node2D.hpp"
#include <vector>
#include <raylib.h>

// ----------------------------------------------------------------------------
// Line2D — draws a polyline through a list of local-space points.
//
// Usage:
//   auto* line = tree.create<Line2D>(parent, "LaserBeam");
//   line->add_point({100, 100});
//   line->add_point({400, 300});
//   line->add_point({700, 100});
//   line->width = 4.0f;
//   line->color = RED;
//
// Points are in the node's LOCAL space.
// The line is transformed by the node's global transform when drawn.
// ----------------------------------------------------------------------------

class Line2D : public Node2D
{
public:
    explicit Line2D(const std::string& p_name = "Line2D");

    // ── Points (local space) ─────────────────────────────────────────────────
    std::vector<Vec2> points;

    // ── Appearance ───────────────────────────────────────────────────────────
    float width      = 2.0f;
    Color color      = WHITE;
    bool  closed     = false;    // connect last point back to first

    // ── Rounded end-caps and joints ──────────────────────────────────────────
    bool  end_caps   = true;     // draw a circle at each endpoint

    // ── Point management ─────────────────────────────────────────────────────
    void add_point   (Vec2 local_pos, int at_index = -1);
    void set_point   (int  index,     Vec2 local_pos);
    void remove_point(int  index);
    void clear_points();
    int  get_point_count() const { return (int)points.size(); }

    // ── Node2D override ───────────────────────────────────────────────────────
    void _draw() override;
};

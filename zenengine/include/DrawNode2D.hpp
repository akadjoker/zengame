#pragma once

#include "Node2D.hpp"
#include <vector>
#include <raylib.h>

// ----------------------------------------------------------------------------
// DrawNode2D — immediate-mode vector drawing node.
//
// All draw calls are recorded each frame in _draw() — call them every frame
// (or rebuild on demand). No retained geometry; just call the draw methods
// inside your node's _draw() override or directly in the game loop.
//
// All positions are in WORLD space (no transform applied).
// Use to_global() / get_global_position() to convert local → world if needed.
//
// Usage:
//   auto* d = tree.create<DrawNode2D>(root, "Debug");
//
//   // In your _draw() or game loop draw phase, BEFORE tree.draw():
//   d->clear();
//   d->draw_circle({400, 300}, 50, RED);
//   d->draw_rect({100, 100}, {200, 200}, GREEN);
//   d->draw_line({0, 0}, {800, 600}, 2.f, WHITE);
// ----------------------------------------------------------------------------

class DrawNode2D : public Node2D
{
public:
    explicit DrawNode2D(const std::string& p_name = "DrawNode2D");

    // ── Clear all recorded commands ─────────────────────────────────────────
    void clear();

    // ── Lines ───────────────────────────────────────────────────────────────
    void draw_line    (Vec2 from, Vec2 to, float width, Color color);
    void draw_polyline(const Vec2* pts, int count, float width, Color color,
                       bool closed = false);

    // ── Circles ─────────────────────────────────────────────────────────────
    void draw_circle      (Vec2 center, float radius, Color color);
    void draw_circle_lines(Vec2 center, float radius, float width, Color color);
    void draw_arc         (Vec2 center, float radius,
                           float start_deg, float end_deg,
                           int segments, float width, Color color);

    // ── Rectangles ──────────────────────────────────────────────────────────
    void draw_rect      (Vec2 top_left, Vec2 bottom_right, Color color);
    void draw_rect_lines(Vec2 top_left, Vec2 bottom_right, float width, Color color);

    // ── Triangles ────────────────────────────────────────────────────────────
    void draw_triangle      (Vec2 a, Vec2 b, Vec2 c, Color color);
    void draw_triangle_lines(Vec2 a, Vec2 b, Vec2 c, float width, Color color);

    // ── Filled polygon ───────────────────────────────────────────────────────
    // pts must be convex and wound CCW for correct fill
    void draw_polygon      (const Vec2* pts, int count, Color fill);
    void draw_polygon_lines(const Vec2* pts, int count, float width, Color color);

    // ── Dots ─────────────────────────────────────────────────────────────────
    void draw_dot(Vec2 center, float radius, Color color);

    // ── Arrows ───────────────────────────────────────────────────────────────
    void draw_arrow(Vec2 from, Vec2 to, float width, float head_size, Color color);

    // ── Node2D override ──────────────────────────────────────────────────────
    void _draw() override;

private:
    enum class CmdType : uint8_t {
        Line, Circle, CircleLines, Arc, Rect, RectLines,
        Triangle, TriangleLines, Dot, Polygon, PolygonLines, Arrow
    };

    struct Cmd {
        CmdType type;
        Color   color;
        float   width      = 1.f;
        float   radius     = 0.f;
        float   start_deg  = 0.f;
        float   end_deg    = 360.f;
        int     segments   = 32;
        float   head_size  = 10.f;
        // up to 4 inline points; polygon pts stored in poly_pts
        Vec2    pts[4]     = {};
        int     pt_count   = 0;
        // heap storage for polygons / polylines with > 4 points
        std::vector<Vec2> poly_pts;
    };

    std::vector<Cmd> m_cmds;
};

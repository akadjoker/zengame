#include "DrawNode2D.hpp"
#include <raylib.h>
#include <cmath>

DrawNode2D::DrawNode2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::DrawNode2D;
}

void DrawNode2D::clear()
{
    m_cmds.clear();
}

// ── Record helpers ────────────────────────────────────────────────────────────

void DrawNode2D::draw_line(Vec2 from, Vec2 to, float width, Color color)
{
    Cmd c;
    c.type    = CmdType::Line;
    c.color   = color;
    c.width   = width;
    c.pts[0]  = from;
    c.pts[1]  = to;
    c.pt_count = 2;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_polyline(const Vec2* pts, int count, float width, Color color, bool closed)
{
    if (count < 2) return;
    Cmd c;
    c.type      = CmdType::Line;   // handled in _draw as a loop
    c.color     = color;
    c.width     = width;
    c.pt_count  = -count;          // negative = polyline; |pt_count| = count
    c.segments  = closed ? 1 : 0;  // reuse segments field for closed flag
    c.poly_pts.assign(pts, pts + count);
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_circle(Vec2 center, float radius, Color color)
{
    Cmd c;
    c.type    = CmdType::Circle;
    c.color   = color;
    c.pts[0]  = center;
    c.radius  = radius;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_circle_lines(Vec2 center, float radius, float width, Color color)
{
    Cmd c;
    c.type    = CmdType::CircleLines;
    c.color   = color;
    c.pts[0]  = center;
    c.radius  = radius;
    c.width   = width;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_arc(Vec2 center, float radius,
                           float start_deg, float end_deg,
                           int segments, float width, Color color)
{
    Cmd c;
    c.type      = CmdType::Arc;
    c.color     = color;
    c.pts[0]    = center;
    c.radius    = radius;
    c.start_deg = start_deg;
    c.end_deg   = end_deg;
    c.segments  = segments;
    c.width     = width;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_rect(Vec2 top_left, Vec2 bottom_right, Color color)
{
    Cmd c;
    c.type     = CmdType::Rect;
    c.color    = color;
    c.pts[0]   = top_left;
    c.pts[1]   = bottom_right;
    c.pt_count = 2;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_rect_lines(Vec2 top_left, Vec2 bottom_right, float width, Color color)
{
    Cmd c;
    c.type     = CmdType::RectLines;
    c.color    = color;
    c.pts[0]   = top_left;
    c.pts[1]   = bottom_right;
    c.width    = width;
    c.pt_count = 2;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_triangle(Vec2 a, Vec2 b, Vec2 c_pt, Color color)
{
    Cmd c;
    c.type     = CmdType::Triangle;
    c.color    = color;
    c.pts[0]   = a;
    c.pts[1]   = b;
    c.pts[2]   = c_pt;
    c.pt_count = 3;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_triangle_lines(Vec2 a, Vec2 b, Vec2 c_pt, float width, Color color)
{
    Cmd c;
    c.type     = CmdType::TriangleLines;
    c.color    = color;
    c.pts[0]   = a;
    c.pts[1]   = b;
    c.pts[2]   = c_pt;
    c.width    = width;
    c.pt_count = 3;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_polygon(const Vec2* pts, int count, Color fill)
{
    if (count < 3) return;
    Cmd c;
    c.type     = CmdType::Polygon;
    c.color    = fill;
    c.pt_count = count;
    c.poly_pts.assign(pts, pts + count);
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_polygon_lines(const Vec2* pts, int count, float width, Color color)
{
    if (count < 2) return;
    Cmd c;
    c.type     = CmdType::PolygonLines;
    c.color    = color;
    c.width    = width;
    c.pt_count = count;
    c.poly_pts.assign(pts, pts + count);
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_dot(Vec2 center, float radius, Color color)
{
    Cmd c;
    c.type    = CmdType::Dot;
    c.color   = color;
    c.pts[0]  = center;
    c.radius  = radius;
    m_cmds.push_back(std::move(c));
}

void DrawNode2D::draw_arrow(Vec2 from, Vec2 to, float width, float head_size, Color color)
{
    Cmd c;
    c.type      = CmdType::Arrow;
    c.color     = color;
    c.width     = width;
    c.head_size = head_size;
    c.pts[0]    = from;
    c.pts[1]    = to;
    c.pt_count  = 2;
    m_cmds.push_back(std::move(c));
}

// ── _draw ─────────────────────────────────────────────────────────────────────

void DrawNode2D::_draw()
{
    Node2D::_draw();

    for (const Cmd& c : m_cmds)
    {
        switch (c.type)
        {
        case CmdType::Line:
            if (c.pt_count < 0)
            {
                // polyline
                const int n    = (int)c.poly_pts.size();
                const int segs = c.segments ? n : (n - 1); // closed if segments==1
                for (int i = 0; i < segs; ++i)
                {
                    const Vec2& a = c.poly_pts[i];
                    const Vec2& b = c.poly_pts[(i + 1) % n];
                    DrawLineEx({a.x, a.y}, {b.x, b.y}, c.width, c.color);
                }
            }
            else
            {
                DrawLineEx({c.pts[0].x, c.pts[0].y},
                           {c.pts[1].x, c.pts[1].y}, c.width, c.color);
            }
            break;

        case CmdType::Circle:
            DrawCircleV({c.pts[0].x, c.pts[0].y}, c.radius, c.color);
            break;

        case CmdType::CircleLines:
            DrawCircleLinesV({c.pts[0].x, c.pts[0].y}, c.radius, c.color);
            break;

        case CmdType::Arc:
        {
            // Draw arc as line segments
            const int   n     = std::max(c.segments, 2);
            const float range = c.end_deg - c.start_deg;
            const float step  = range / (float)(n);
            const float r     = c.radius;
            const Vec2& ctr   = c.pts[0];
            for (int i = 0; i < n; ++i)
            {
                const float a0 = (c.start_deg + step * i)       * (M_PI_F / 180.f);
                const float a1 = (c.start_deg + step * (i + 1)) * (M_PI_F / 180.f);
                Vector2 p0 = {ctr.x + cosf(a0) * r, ctr.y + sinf(a0) * r};
                Vector2 p1 = {ctr.x + cosf(a1) * r, ctr.y + sinf(a1) * r};
                DrawLineEx(p0, p1, c.width, c.color);
            }
            break;
        }

        case CmdType::Rect:
        {
            const int x = (int)c.pts[0].x, y = (int)c.pts[0].y;
            const int w = (int)(c.pts[1].x - c.pts[0].x);
            const int h = (int)(c.pts[1].y - c.pts[0].y);
            DrawRectangle(x, y, w, h, c.color);
            break;
        }

        case CmdType::RectLines:
        {
            const Rectangle r = {c.pts[0].x, c.pts[0].y,
                                  c.pts[1].x - c.pts[0].x,
                                  c.pts[1].y - c.pts[0].y};
            DrawRectangleLinesEx(r, c.width, c.color);
            break;
        }

        case CmdType::Triangle:
            DrawTriangle({c.pts[0].x, c.pts[0].y},
                         {c.pts[1].x, c.pts[1].y},
                         {c.pts[2].x, c.pts[2].y}, c.color);
            break;

        case CmdType::TriangleLines:
            DrawLineEx({c.pts[0].x, c.pts[0].y}, {c.pts[1].x, c.pts[1].y}, c.width, c.color);
            DrawLineEx({c.pts[1].x, c.pts[1].y}, {c.pts[2].x, c.pts[2].y}, c.width, c.color);
            DrawLineEx({c.pts[2].x, c.pts[2].y}, {c.pts[0].x, c.pts[0].y}, c.width, c.color);
            break;

        case CmdType::Dot:
            DrawCircleV({c.pts[0].x, c.pts[0].y}, c.radius, c.color);
            break;

        case CmdType::Polygon:
        {
            const int n = (int)c.poly_pts.size();
            if (n < 3) break;

            // Centroid-fan triangulation: robust for any simple polygon
            // where the centroid lies inside (convex, stars, regular concave).
            float cx = 0.f, cy = 0.f;
            for (const Vec2& p : c.poly_pts) { cx += p.x; cy += p.y; }
            cx /= n; cy /= n;

            for (int i = 0; i < n; ++i)
            {
                const Vec2& b = c.poly_pts[i];
                const Vec2& a = c.poly_pts[(i + 1) % n];
                DrawTriangle({cx, cy}, {a.x, a.y}, {b.x, b.y}, c.color);
            }
            break;
        }

        case CmdType::PolygonLines:
        {
            const int n = (int)c.poly_pts.size();
            for (int i = 0; i < n; ++i)
            {
                const Vec2& a = c.poly_pts[i];
                const Vec2& b = c.poly_pts[(i + 1) % n];
                DrawLineEx({a.x, a.y}, {b.x, b.y}, c.width, c.color);
            }
            break;
        }

        case CmdType::Arrow:
        {
            const Vec2& from = c.pts[0];
            const Vec2& to   = c.pts[1];
            DrawLineEx({from.x, from.y}, {to.x, to.y}, c.width, c.color);

            // Arrowhead
            const float dx  = to.x - from.x;
            const float dy  = to.y - from.y;
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) break;

            const float ux = dx / len, uy = dy / len;
            const float h  = c.head_size;
            // Base of arrowhead
            const float bx = to.x - ux * h;
            const float by = to.y - uy * h;
            // Perpendicular
            const float px = -uy * h * 0.4f;
            const float py =  ux * h * 0.4f;

            DrawTriangle({to.x, to.y},
                         {bx - px, by - py},
                         {bx + px, by + py}, c.color);
            break;
        }
        }
    }
}

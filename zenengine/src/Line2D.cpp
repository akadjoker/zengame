 #include "Line2D.hpp"

Line2D::Line2D(const std::string& p_name) : Node2D(p_name)
{
    node_type = NodeType::Line2D;
}

// ── Point management ──────────────────────────────────────────────────────────

void Line2D::add_point(Vec2 local_pos, int at_index)
{
    if (at_index < 0 || at_index >= (int)points.size())
        points.push_back(local_pos);
    else
        points.insert(points.begin() + at_index, local_pos);
}

void Line2D::set_point(int index, Vec2 local_pos)
{
    if (index >= 0 && index < (int)points.size())
        points[index] = local_pos;
}

void Line2D::remove_point(int index)
{
    if (index >= 0 && index < (int)points.size())
        points.erase(points.begin() + index);
}

void Line2D::clear_points()
{
    points.clear();
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void Line2D::_draw()
{
    Node2D::_draw();

    const int n = (int)points.size();
    if (n < 2) return;

    // Draw segments — convert each point local→world
    const int segs = closed ? n : (n - 1);
    for (int i = 0; i < segs; ++i)
    {
        const Vec2 a = to_global(points[i]);
        const Vec2 b = to_global(points[(i + 1) % n]);
        DrawLineEx({a.x, a.y}, {b.x, b.y}, width, color);
    }

    // End-caps (circles at the endpoints so joints look clean)
    if (end_caps)
    {
        const float r = width * 0.5f;
        for (int i = 0; i < (closed ? n : 2); ++i)
        {
            const int   idx = (i == 0) ? 0 : (closed ? i : n - 1);
            const Vec2  p   = to_global(points[idx]);
            DrawCircleV({p.x, p.y}, r, color);
            if (!closed && i == 1) break;  // only first + last for open lines
        }
        if (!closed)
        {
            const Vec2 last = to_global(points[n - 1]);
            DrawCircleV({last.x, last.y}, r, color);
        }
    }
}

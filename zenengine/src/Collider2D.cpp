#include "pch.hpp"
#include "Collider2D.hpp"

Collider2D::Collider2D(const std::string& p_name)
    : Node2D(p_name)
{
    points = {
        Vec2(0.0f, 0.0f),
        Vec2(32.0f, 0.0f),
        Vec2(32.0f, 32.0f),
        Vec2(0.0f, 32.0f)
    };
}

void Collider2D::get_world_segment(Vec2& p0, Vec2& p1) const
{
    const Matrix2D xf = get_global_transform();
    p0 = points.size() > 0 ? xf.TransformCoords(points[0]) : Vec2();
    p1 = points.size() > 1 ? xf.TransformCoords(points[1]) : Vec2();
}

Rectangle Collider2D::get_world_aabb() const
{
    if (shape == ShapeType::Circle)
    {
        const Vec2 center = get_world_center();
        const float r = get_world_radius();
        return Rectangle{center.x - r, center.y - r, r * 2.0f, r * 2.0f};
    }

    
    if (shape == ShapeType::Segment)
    {
        Vec2 p0, p1;
        get_world_segment(p0, p1);
        const float x0 = std::min(p0.x, p1.x);
        const float y0 = std::min(p0.y, p1.y);
        const float x1 = std::max(p0.x, p1.x);
        const float y1 = std::max(p0.y, p1.y);
        // Segment is zero-thickness in narrow phase, so broad-phase needs a
        // configurable padding to behave like a usable static blocker/ramp.
        const float pad = std::max(segment_padding, 1.0f);
        return Rectangle{ x0 - pad, y0 - pad, (x1 - x0) + pad * 2.0f, (y1 - y0) + pad * 2.0f };
    }
    std::vector<Vec2> poly;
    get_world_polygon(poly);
    if (poly.empty())
    {
        const Vec2 p = get_global_position();
        return Rectangle{p.x, p.y, 0.0f, 0.0f};
    }

    float min_x = poly[0].x;
    float min_y = poly[0].y;
    float max_x = poly[0].x;
    float max_y = poly[0].y;
    for (const Vec2& p : poly)
    {
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }
    return Rectangle{min_x, min_y, max_x - min_x, max_y - min_y};
}

Vec2 Collider2D::get_world_center() const
{
    const Matrix2D m = get_global_transform();

    if (shape == ShapeType::Circle)
    {
        return Vec2(m.tx, m.ty);
    }

    if (shape == ShapeType::Rectangle)
    {
        return m.TransformCoords(Vec2(size.x * 0.5f, size.y * 0.5f));
    }

    if (shape == ShapeType::Segment)
    {
        const Vec2 a = points.size() > 0 ? points[0] : Vec2();
        const Vec2 b = points.size() > 1 ? points[1] : a;
        return m.TransformCoords((a + b) * 0.5f);
    }

    if (shape == ShapeType::Polygon && !points.empty())
    {
        Vec2 acc(0.0f, 0.0f);
        for (const Vec2& p : points)
        {
            acc += p;
        }
        acc /= static_cast<float>(points.size());
        return m.TransformCoords(acc);
    }

    return Vec2(m.tx, m.ty);
}

float Collider2D::get_world_radius() const
{
    const Matrix2D m = get_global_transform();
    const float sx = std::sqrt(std::max(0.0f, m.a * m.a + m.b * m.b));
    const float sy = std::sqrt(std::max(0.0f, m.c * m.c + m.d * m.d));
    return radius * std::max(sx, sy);
}

void Collider2D::get_world_polygon(std::vector<Vec2>& out_points) const
{
    out_points.clear();
    const Matrix2D m = get_global_transform();

    if (shape == ShapeType::Rectangle)
    {
        out_points.push_back(m.TransformCoords(Vec2(0.0f, 0.0f)));
        out_points.push_back(m.TransformCoords(Vec2(size.x, 0.0f)));
        out_points.push_back(m.TransformCoords(Vec2(size.x, size.y)));
        out_points.push_back(m.TransformCoords(Vec2(0.0f, size.y)));
        return;
    }

    if (shape == ShapeType::Polygon)
    {
        out_points.reserve(points.size());
        for (const Vec2& p : points)
        {
            out_points.push_back(m.TransformCoords(p));
        }
    }
}

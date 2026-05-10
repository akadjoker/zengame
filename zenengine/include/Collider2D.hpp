#pragma once

#include "pch.hpp"
#include "Node2D.hpp"
#include <raylib.h>

class Collider2D : public Node2D
{
public:

    enum class ShapeType
    {
        Rectangle,
        Circle,
        Polygon,
        Segment
    };

    explicit Collider2D(const std::string& p_name = "Collider2D");

    ShapeType shape = ShapeType::Rectangle;
    Vec2 size = Vec2(32.0f, 32.0f);
    float radius = 16.0f;
    std::vector<Vec2> points;
    bool one_sided = false;  

    void get_world_segment(Vec2& p0, Vec2& p1) const;
    Rectangle get_world_aabb() const;
    Vec2 get_world_center() const;
    float get_world_radius() const;
    void get_world_polygon(std::vector<Vec2>& out_points) const;
};
